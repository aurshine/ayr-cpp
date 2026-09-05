#ifndef AYR_NET_HTTP_URI_HPP
#define AYR_NET_HTTP_URI_HPP

#include "../../air/Dict.hpp"

namespace ayr
{
	namespace net
	{
		// 检查端口号的合法性，端口号范围为 [1,65535]
		def check_port(int port) { return port > 0 && port <= 65535; }

		// scheme://host[:port][/path][?query][#fragment]
		class Uri
		{
			using self = Uri;

			// uri端口号
			int port_;

			// uri方案(协议)
			CString scheme_;

			// uri主机名
			CString host_;

			// uri路径
			CString path_;

			// uri查询参数
			Dict<CString, CString> query_dict_;

			// uri片段
			CString fragment_;
		public:
			Uri() : scheme_(), host_(), port_(-1), path_("/"), query_dict_(), fragment_() {}

			Uri(const self& other) :
				scheme_(other.scheme_),
				host_(other.host_),
				port_(other.port_),
				path_(other.path_),
				query_dict_(other.query_dict_),
				fragment_(other.fragment_) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, other);
			}

			// uri的方案
			const CString& scheme() const { return scheme_; }

			// 设置uri的方案
			const CString& scheme(const CString& scheme) { return scheme_ = scheme.lower(); }

			// uri的主机名
			const CString& host() const { return host_; }

			// 设置uri的主机名
			const CString& host(const CString& host) { return host_ = host; }

			// uri的端口号
			int port() const { return port_; }

			// 设置uri的端口号
			int port(int port) 
			{ 
				if (!check_port(port))
					ValueError(vstr("Invalid HTTP port: ") + cstr(port));
				return port_ = port;
			}

			// port 是否有效
			bool valid_port() const { return check_port(port_); }

			// uri的路径
			const CString& path() const { return path_; }

			// 设置uri的路径
			const CString& path(const CString& path) { return path_ = path; }

			// uri的查询参数的字符串形式
			CString query() const
			{
				DynArray<CString> query_list;
				for (auto& [key, value] : queries())
					query_list.append(CString::cjoin(arr(key, vstr("="), value)));
				return vstr("&").join(query_list);
			}

			// uri的查询参数字典
			const Dict<CString, CString>& queries() const { return query_dict_; }

			// 添加查询参数
			const Dict<CString, CString>& add_query(const CString& key, const CString& value)
			{
				query_dict_.insert(key, value);
				return query_dict_;
			}

			// uri的片段
			const CString& fragment() const { return fragment_; }

			// 设置uri的片段
			const CString& fragment(const CString& fragment) { return fragment_ = fragment; }

			void __repr__(Buffer& buffer) const
			{
				if (!scheme_.empty())
					buffer << scheme_ << "://";
				if (!host_.empty())
					buffer << host_;
				if (valid_port())
					buffer << ":" << port_;
				if (!path_.empty())
					buffer << path_;
				if (!query_dict_.empty())
					buffer << "?" << query();
				if (!fragment_.empty())
					buffer << "#" << fragment_;
			}
		};

		def _char_split(const CString& s, char sp)
		{
			c_size pos = s.index(sp);
			if (pos == -1) return std::make_pair(std::make_pair(s, vstr("")), 1);
			return std::make_pair(std::make_pair(s.vslice(0, pos), s.vslice(pos + 1)), 2);
		}

		/*
		* @brief 找到uri_str中uri分割字符的下标
		*
		* 分割字符包括 '/' '#' '?'
		*
		* @param uri_str 待解析的uri字符串
		*
		* @return uri分割字符的下标，如果没有找到，则返回uri_str的长度
		*/
		def _find_sep(const CString& uri_str)
		{
			for (auto&& [i, c] : enumerate(uri_str))
				if (c == '/' || c == '#' || c == '?')
					return i;
			return uri_str.size();
		}

		def _parse_scheme(Uri& uri, CString& uri_str_view)
		{
			if (uri_str_view.empty()) return;

			c_size i = uri_str_view.index(vstr("://"));
			if (i != -1)
			{
				uri.scheme(uri_str_view.vslice(0, i));
				uri_str_view = uri_str_view.vslice(i + 3);
			}
		}

		def _parse_host_port(Uri& uri, CString& uri_str_view)
		{
			if (uri_str_view.empty()) return;

			c_size i = 0;
			for (auto& c : uri_str_view)
			{
				if (c == '/' || c == '#' || c == '?')
					break;
				++i;
			}

			auto [host_port, cnt] = _char_split(uri_str_view.vslice(0, i), ':');

			uri.host(host_port.first);
			if (cnt > 1)
				uri.port(host_port.second.toint());
			uri_str_view = uri_str_view.vslice(i);
		}

		def _parse_path(Uri& uri, CString& uri_str_view)
		{
			c_size i = 0;
			for (auto& c : uri_str_view)
			{
				if (c == '#' || c == '?')
					break;
				++i;
			}
			if (i == 0)
				uri.path(vstr("/"));
			else
				uri.path(uri_str_view.vslice(0, i));
			// 这里需要保留 '#' '?'
			uri_str_view = uri_str_view.vslice(i);
		}

		def _parse_query(Uri& uri, CString& uri_str_view)
		{
			if (uri_str_view.empty() || uri_str_view[0] != '?') return;

			c_size i = uri_str_view.index(vstr("#"));
			if (i == -1)
				i = uri_str_view.size();
			CString queries = uri_str_view.vslice(1, i);

			while (true)
			{
				auto [kv_other, cnt] = _char_split(queries, '&');
				auto [key_value, cnt2] = _char_split(kv_other.first, '=');
				if (cnt2 != 2)
					ValueError(vstr("Invalid query string: ") + cstr(kv_other.first));
				uri.add_query(key_value.first, key_value.second);

				if (cnt == 1) break;
				queries = kv_other.second;
			}
			
			uri_str_view = uri_str_view.vslice(i);
		}

		void _parse_fragment(Uri& uri, CString& uri_str_view)
		{
			if (!uri_str_view.empty() && uri_str_view.startswith(vstr("#")))
				uri.fragment(uri_str_view.vslice(1));
		}

		/*
		* @brief 解析uri字符串，返回Uri对象
		* 
		* @details 会采用https协议作为默认scheme，会根据scheme自动设置默认端口号，http为80，https为443
		* 
		* @param main_uri_str 待解析的uri字符串
		*/ 
		def uri(const CString& main_uri_str) -> Uri
		{
			// 创建视图
			CString uri_str_view = vstr(main_uri_str);
			Uri res;
			_parse_scheme(res, uri_str_view);
			_parse_host_port(res, uri_str_view);
			_parse_path(res, uri_str_view);
			_parse_query(res, uri_str_view);
			_parse_fragment(res, uri_str_view);

			// 默认使用https协议
			if (res.scheme().empty())
				res.scheme(vstr("https"));

			// 根据scheme自动设置默认端口号
			if (!res.valid_port())
			{
				if (res.scheme() == vstr("http"))
					res.port(80);
				else if (res.scheme() == vstr("https"))
					res.port(443);
			}
			
			return res;
		}
	}
}
#endif // AYR_NET_HTTP_URI_HPP