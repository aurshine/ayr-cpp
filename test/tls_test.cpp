#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include <openssl/rsa.h>

#include <ayr/net/TlsLayer.hpp>
#include <ayr/base/utest.hpp>

using namespace ayr;
using namespace ayr::net;

namespace
{
	using SSLContext = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
	using SSLKey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
	using SSLCertificate = std::unique_ptr<X509, decltype(&X509_free)>;

	struct TestTlsContexts
	{
		SSLContext client;
		SSLContext server;

		TestTlsContexts(SSLContext client, SSLContext server) :
			client(std::move(client)), server(std::move(server)) {}
	};

	void require_ssl(bool condition, const char* operation)
	{
		if (!condition)
			SSLError(vstr(operation) + vstr(": ") + ssl_error_msg());
	}

	TestTlsContexts create_test_contexts()
	{
		std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> key_context(
			EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr),
			&EVP_PKEY_CTX_free
		);
		require_ssl(key_context != nullptr, "Failed to create test key context");
		require_ssl(EVP_PKEY_keygen_init(key_context.get()) == 1, "Failed to initialize test key generation");
		require_ssl(
			EVP_PKEY_CTX_set_rsa_keygen_bits(key_context.get(), 2048) == 1,
			"Failed to configure test key"
		);

		EVP_PKEY* generated_key = nullptr;
		require_ssl(
			EVP_PKEY_keygen(key_context.get(), &generated_key) == 1,
			"Failed to generate test key"
		);
		SSLKey key(generated_key, &EVP_PKEY_free);

		SSLCertificate certificate(X509_new(), &X509_free);
		require_ssl(certificate != nullptr, "Failed to create test certificate");
		require_ssl(X509_set_version(certificate.get(), 2) == 1, "Failed to set certificate version");
		require_ssl(
			ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1) == 1,
			"Failed to set certificate serial number"
		);
		require_ssl(X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -60) != nullptr, "Failed to set notBefore");
		require_ssl(X509_gmtime_adj(X509_getm_notAfter(certificate.get()), 3600) != nullptr, "Failed to set notAfter");
		require_ssl(X509_set_pubkey(certificate.get(), key.get()) == 1, "Failed to set certificate public key");

		X509_NAME* subject = X509_get_subject_name(certificate.get());
		require_ssl(subject != nullptr, "Failed to get certificate subject");
		require_ssl(
			X509_NAME_add_entry_by_txt(
				subject,
				"CN",
				MBSTRING_ASC,
				reinterpret_cast<const unsigned char*>("localhost"),
				-1,
				-1,
				0
			) == 1,
			"Failed to set certificate common name"
		);
		require_ssl(
			X509_set_issuer_name(certificate.get(), subject) == 1,
			"Failed to set certificate issuer"
		);
		require_ssl(
			X509_sign(certificate.get(), key.get(), EVP_sha256()) > 0,
			"Failed to sign test certificate"
		);

		SSLContext server_context(SSL_CTX_new(TLS_server_method()), &SSL_CTX_free);
		SSLContext client_context(SSL_CTX_new(TLS_client_method()), &SSL_CTX_free);
		require_ssl(server_context != nullptr, "Failed to create server context");
		require_ssl(client_context != nullptr, "Failed to create client context");
		require_ssl(
			SSL_CTX_set_min_proto_version(server_context.get(), TLS1_2_VERSION) == 1,
			"Failed to set server TLS version"
		);
		require_ssl(
			SSL_CTX_set_min_proto_version(client_context.get(), TLS1_2_VERSION) == 1,
			"Failed to set client TLS version"
		);
		require_ssl(
			SSL_CTX_use_certificate(server_context.get(), certificate.get()) == 1,
			"Failed to configure server certificate"
		);
		require_ssl(
			SSL_CTX_use_PrivateKey(server_context.get(), key.get()) == 1,
			"Failed to configure server private key"
		);
		require_ssl(
			X509_STORE_add_cert(SSL_CTX_get_cert_store(client_context.get()), certificate.get()) == 1,
			"Failed to trust test certificate"
		);
		SSL_CTX_set_verify(client_context.get(), SSL_VERIFY_PEER, nullptr);

		return TestTlsContexts(std::move(client_context), std::move(server_context));
	}

	void transfer_fragment(Buffer& encrypted, TlsLayer& destination, c_size fragment_size)
	{
		if (encrypted.readable_size() == 0)
			return;

		c_size bytes = std::min(encrypted.readable_size(), fragment_size);
		destination.read_buffer().append_bytes(encrypted.peek(), bytes);
		encrypted.retrieve(bytes);
	}

	void complete_handshake(
		TlsLayer& client,
		TlsLayer& server,
		const CString& hostname,
		c_size fragment_size
	)
	{
		Buffer client_to_server, server_to_client;
		bool client_complete = false, server_complete = false, handshake_valid = true;

		for (int attempt = 0; attempt < 10000 && (!client_complete || !server_complete); ++attempt)
		{
			TlsResult client_result = client.handshake_client(client_to_server, hostname);
			if (
				client_result.state == TlsState::Error
				|| client_result.state == TlsState::Closed
			)
			{
				handshake_valid = false;
				break;
			}
			client_complete = client_result.state == TlsState::Complete;
			transfer_fragment(client_to_server, server, fragment_size);

			TlsResult server_result = server.handshake_server(server_to_client);
			if (
				server_result.state == TlsState::Error
				|| server_result.state == TlsState::Closed
			)
			{
				handshake_valid = false;
				break;
			}
			server_complete = server_result.state == TlsState::Complete;
			transfer_fragment(server_to_client, client, fragment_size);
		}

		while (client_to_server.readable_size() > 0)
			transfer_fragment(client_to_server, server, fragment_size);
		while (server_to_client.readable_size() > 0)
			transfer_fragment(server_to_client, client, fragment_size);

		UTEST_EXPECT(handshake_valid);
		UTEST_EXPECT(client_complete);
		UTEST_EXPECT(server_complete);
		UTEST_EXPECT(client_to_server.readable_size() == 0);
		UTEST_EXPECT(server_to_client.readable_size() == 0);
	}
}

int main()
{
	TestTlsContexts contexts = create_test_contexts();
	TlsLayer client(contexts.client.get()), server(contexts.server.get());

	UTEST_SCOPE("测试握手可以跨越大量不完整的 TLS record 继续推进。")
	{
		complete_handshake(client, server, vstr("localhost"), 31);
	};

	UTEST_SCOPE("测试 read_buffer 交给底层读取前至少保留 8192 字节可写空间。")
	{
		UTEST_EXPECT(client.read_buffer().writeable_size() >= 8192);
	};

	Buffer decrypted, response_records;
	UTEST_SCOPE("测试 TLS 加密数据可按网络分片解密并排空内部 record。")
	{
		std::string payload;
		payload.reserve(40000);
		for (int index = 0; index < 40000; ++index)
			payload.push_back(static_cast<char>('a' + index % 26));

		Buffer plaintext, encrypted;
		plaintext.append_bytes(payload.data(), static_cast<c_size>(payload.size()));
		while (plaintext.readable_size() > 0)
		{
			TlsResult encrypt_result = client.encrypt(plaintext, encrypted);
			UTEST_EXPECT(encrypt_result.state == TlsState::Complete);
			UTEST_EXPECT(encrypt_result.bytes > 0);
		}

		bool decrypt_valid = true;
		for (int attempt = 0; attempt < 10000; ++attempt)
		{
			transfer_fragment(encrypted, server, 37);
			TlsResult decrypt_result = server.decrypt(decrypted, response_records);
			if (
				decrypt_result.state == TlsState::Error
				|| decrypt_result.state == TlsState::Closed
			)
			{
				decrypt_valid = false;
				break;
			}

			if (
				encrypted.readable_size() == 0
				&& decrypt_result.state == TlsState::WantRead
			)
				break;
		}

		UTEST_EXPECT(decrypt_valid);
		UTEST_EXPECT(decrypted.readable_size() == static_cast<c_size>(payload.size()));
		UTEST_EXPECT(
			std::memcmp(decrypted.peek(), payload.data(), payload.size()) == 0
		);
	};

	UTEST_SCOPE("测试 close_notify 表现为有序关闭。")
	{
		Buffer close_notify;
		TlsResult shutdown_result = client.shutdown(close_notify);
		UTEST_EXPECT(shutdown_result.state == TlsState::Complete);
		while (close_notify.readable_size() > 0)
			transfer_fragment(close_notify, server, 7);

		TlsResult close_result = server.decrypt(decrypted, response_records);
		UTEST_EXPECT(close_result.state == TlsState::Closed);
	};

	UTEST_SCOPE("测试相同证书不能通过错误主机名的校验。")
	{
		TlsLayer mismatched_client(contexts.client.get()), mismatched_server(contexts.server.get());
		Buffer client_to_server, server_to_client;
		bool hostname_rejected = false;
		for (int attempt = 0; attempt < 1000 && !hostname_rejected; ++attempt)
		{
			TlsResult client_result = mismatched_client.handshake_client(
				client_to_server,
				vstr("wrong.example")
			);
			hostname_rejected = client_result.state == TlsState::Error;
			transfer_fragment(client_to_server, mismatched_server, 4096);
			if (hostname_rejected)
				break;

			TlsResult server_result = mismatched_server.handshake_server(server_to_client);
			UTEST_EXPECT(server_result.state != TlsState::Error);
			transfer_fragment(server_to_client, mismatched_client, 4096);
		}
		UTEST_EXPECT(hostname_rejected);
	};

	return UTEST_COMPLETE();
}
