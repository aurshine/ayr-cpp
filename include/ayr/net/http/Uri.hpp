#ifndef AYR_NET_HTTP_URI_HPP
#define AYR_NET_HTTP_URI_HPP

#include "../../air/Dict.hpp"

namespace ayr
{
	namespace net
	{
		// scheme://host[:port][/path][?query][#fragment]
		class Uri : Object<Uri>
		{
			using self = Uri;

			using super = Object<Uri>;

			// uri方案(协议)
			Atring scheme_;

			// uri主机名
			Atring host_;

			// uri端口号
			Atring port_;

			// uri路径
			Atring path_;

			// uri查询参数
			Dict<Atring, Atring> query_dict_;

			// uri片段
			Atring fragment_;
		public:
			Uri() : scheme_(), host_(), port_(), path_(), query_dict_(), fragment_() {}

			Uri(const self& other) :
				scheme_(other.scheme_.clone()),
				host_(other.host_.clone()),
				port_(other.port_.clone()),
				path_(other.path_.clone()),
				query_dict_(other.query_dict_),
				fragment_(other.fragment_.clone()) {
			}

			Uri(self&& other) noexcept :
				scheme_(std::move(other.scheme_)),
				host_(std::move(other.host_)),
				port_(std::move(other.port_)),
				path_(std::move(other.path_)),
				query_dict_(std::move(other.query_dict_)),
				fragment_(std::move(other.fragment_)) {
			}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, other);
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			// uri的方案
			const Atring& scheme() const { return scheme_; }

			// 设置uri的方案
			const Atring& scheme(const Atring& scheme) { return scheme_ = scheme.clone(); }

			// uri的主机名
			const Atring& host() const { return host_; }

			// 设置uri的主机名
			const Atring& host(const Atring& host) { return host_ = host.clone(); }

			// uri的端口号
			const Atring& port() const { return port_; }

			// 设置uri的端口号
			const Atring& port(const Atring& port) { return port_ = port.clone(); }

			// uri的路径
			const Atring& path() const { return path_; }

			// 设置uri的路径
			const Atring& path(const Atring& path) { return path_ = path.clone(); }

			// uri的查询参数的字符串形式
			Atring query() const
			{
				DynArray<Atring> query_list;
				for (auto& [key, value] : queries())
					query_list.append("="as.join(arr(key, value)));
				return "&"as.join(query_list);
			}

			// uri的查询参数字典
			const Dict<Atring, Atring>& queries() const { return query_dict_; }

			// 添加查询参数
			const Dict<Atring, Atring>& add_query(const Atring& key, const Atring& value)
			{
				query_dict_.insert(key.clone(), value.clone());
				return query_dict_;
			}

			// uri的片段
			const Atring& fragment() const { return fragment_; }

			// 设置uri的片段
			const Atring& fragment(const Atring& fragment) { return fragment_ = fragment.clone(); }

			void __repr__(Buffer& buffer) const
			{
				if (!scheme_.empty())
					buffer << scheme_ << "://";
				if (!host_.empty())
					buffer << host_;
				if (!port_.empty())
					buffer << ":" << port_;
				if (!path_.empty())
					buffer << path_;
				if (!query_dict_.empty())
					buffer << "?" << query();
				if (!fragment_.empty())
					buffer << "#" << fragment_;
			}
		};

		/*
		* @brief 找到uri_str中uri分割字符的下标
		*
		* 分割字符包括 '/' '#' '?'
		*
		* @param uri_str 待解析的uri字符串
		*
		* @return uri分割字符的下标，如果没有找到，则返回uri_str的长度
		*/
		def _find_sep(const Atring& uri_str)
		{
			for (auto&& [i, c] : enumerate(uri_str))
				if (c == '/' || c == '#' || c == '?')
					return i;
			return uri_str.size();
		}

		def _parse_scheme(Uri& uri, Atring& uri_str)
		{
			if (uri_str.empty()) return;

			c_size i = uri_str.index("://"as);
			if (i != -1)
			{
				uri.scheme(uri_str.slice(0, i));
				uri_str = uri_str.vslice(i + 3);
			}
		}

		def _parse_host_port(Uri& uri, Atring& uri_str)
		{
			if (uri_str.empty()) return;

			c_size i = 0;
			for (auto& c : uri_str)
			{
				if (c == '/' || c == '#' || c == '?')
					break;
				++i;
			}

			Array<Atring> host_port = uri_str.vslice(0, i).split(":"as, 1);
			uri.host(host_port[0].clone());
			if (host_port.size() == 2)
				uri.port(host_port[1].clone());
			uri_str = uri_str.vslice(i);
		}

		def _parse_path(Uri& uri, Atring& uri_str)
		{
			c_size i = 0;
			for (auto& c : uri_str)
			{
				if (c == '#' || c == '?')
					break;
				++i;
			}
			if (i == 0)
				uri.path("/"as);
			else
				uri.path(uri_str.slice(0, i));
			// 这里需要保留 '#' '?'
			uri_str = uri_str.vslice(i);
		}

		def _parse_query(Uri& uri, Atring& uri_str)
		{
			if (uri_str.empty() || uri_str[0] != '?') return;

			c_size i = uri_str.index("#"as);
			if (i == -1)
				i = uri_str.size();
			Atring queries = uri_str.vslice(1, i);
			for (auto& kv : queries.split("&"as))
			{
				Array<Atring> key_value = kv.split("="as, 1);
				if (key_value.size() != 2)
					ValueError("Invalid query string: "as + kv);
				uri.add_query(key_value[0].clone(), key_value[1].clone());
			}
			// 这里跳过 '#'
			uri_str = uri_str.vslice(i + 1);
		}

		// 解析uri字符串，返回Uri对象
		def uri(const Atring& main_uri_str) -> Uri
		{
			Atring uri_str = main_uri_str.vslice(0);
			Uri res;
			_parse_scheme(res, uri_str);
			_parse_host_port(res, uri_str);
			_parse_path(res, uri_str);
			_parse_query(res, uri_str);
			res.fragment(uri_str.clone());
			return res;
		}
	}
}
#endif // AYR_NET_HTTP_URI_HPP