#ifndef AYR_NET_HTTP_REQUEST_HPP
#define AYR_NET_HTTP_REQUEST_HPP

#include "../Socket.hpp"
#include "../../Atring.hpp"

namespace ayr
{
	class RequestParser : public Object<RequestParser>
	{
		Atring req_line_, headers_, more_data_;

		bool req_line_finised = false, headers_finised = false;
	public:
		Dict<Atring, Atring> headers;

		Atring method, uri, version;

		RequestParser() {}

		void reset()
		{
			req_line_ = more_data_ = headers_ = "";
			headers.clear();
			req_line_finised = headers_finised = false;
		}

		bool parse(const Atring& data)
		{
			more_data_ += data;

			if (!req_line_finised && !parse_req_line())
				return false;
			else
			{
				auto&& line_parts = req_line_.split(" ");
				method = line_parts[0];
				uri = line_parts[1];
				version = line_parts[2];
			}

			if (!headers_finised && !parse_headers())
				return false;
			else
			{
				for (const Atring& kv_str : headers_.split("\r\n"))
				{
					auto&& kv = kv_str.split(":");
					if (kv.size() == 2)
						headers.insert(kv[0].lower().strip(), kv[1].lower().strip());
				}
			}
			return true;
		}
	private:
		bool parse_req_line()
		{
			c_size pos = more_data_.find("\r\n");
			if (pos == -1)
			{
				req_line_ += more_data_;
			}
			else
			{
				req_line_ += more_data_.slice(0, pos);
				more_data_ = more_data_.slice(pos + 2);
				req_line_finised = true;
			}

			return req_line_finised;
		}

		bool parse_headers()
		{
			c_size pos = more_data_.find("\r\n\r\n");
			if (pos == -1)
			{
				headers_ += more_data_;
			}
			else
			{
				headers_ += more_data_.slice(0, pos);
				more_data_ = more_data_.slice(pos + 4);
				headers_finised = true;
			}

			return headers_finised;
		}
	};
}
#endif // AYR_NET_HTTP_REQUEST_HPP