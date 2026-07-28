#ifndef AYR_NET_HTTP_HEADERS_HPP
#define AYR_NET_HTTP_HEADERS_HPP

#include "../../air/DynArray.hpp"

namespace ayr
{
	namespace net
	{
		/**
		 * @brief HTTP 头字段集合
		 *
		 * @details 字段名按 ASCII 大小写不敏感方式查找，并保留重复字段及原始名称。
		 */
		class HttpHeaders
		{
			using self = HttpHeaders;

			using Entry = std::pair<Atring, Atring>;

			DynArray<Entry> entries_;
		public:
			HttpHeaders() : entries_() {}

			HttpHeaders(const self& other) : entries_()
			{
				for (const auto& [key, value] : other.entries_)
					entries_.append(Entry(key.clone(), value.clone()));
			}

			HttpHeaders(self&& other) noexcept : entries_(std::move(other.entries_)) {}

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

			// 头字段数量，重复字段分别计数。
			c_size size() const { return entries_.size(); }

			// 是否没有头字段。
			bool empty() const { return entries_.empty(); }

			// 判断是否包含指定字段名。
			bool contains(const Atring& key) const { return find_index(key) != -1; }

			// 获取指定字段，不存在时抛出异常。
			const Atring& get(const Atring& key) const
			{
				c_size index = find_index(key);
				if (index != -1)
					return entries_[index].second;
				RuntimeError(ayr::format("HTTP header '{}' not found.", key));
				return None;
			}

			// 获取指定字段，不存在时返回默认值。
			const Atring& get(const Atring& key, const Atring& default_value) const
			{
				c_size index = find_index(key);
				return ifelse(index == -1, default_value, entries_[index].second);
			}

			/**
			 * @brief 设置头字段
			 *
			 * @details 已有的同名字段会被替换。
			 */
			void insert(const Atring& key, const Atring& value)
			{
				validate(key, value);
				entries_.append(Entry(key.clone(), value.clone()));
			}

			// 删除头字段。
			void pop(const Atring& key)
			{
				c_size index = find_index(key);
				if (index != -1)
					entries_.pop(find_index(key));
			}

			// 清空全部头字段。
			void clear() { entries_.clear(); }

			// 按接收顺序访问全部字段。
			const DynArray<Entry>& items() const { return entries_; }

			auto begin() const { return entries_.begin(); }

			auto end() const { return entries_.end(); }

		private:
			// HTTP 字段名只允许 ASCII，此处使用 Atring 的大小写转换统一比较。
			static bool key_equal(const Atring& left, const Atring& right)
			{
				return left.lower() == right.lower();
			}

			static bool valid_name_char(int ch)
			{
				return
					(ch >= '0' && ch <= '9')
					|| (ch >= 'A' && ch <= 'Z')
					|| (ch >= 'a' && ch <= 'z')
					|| ch == '!' || ch == '#' || ch == '$' || ch == '%'
					|| ch == '&' || ch == '\'' || ch == '*' || ch == '+'
					|| ch == '-' || ch == '.' || ch == '^' || ch == '_'
					|| ch == '`' || ch == '|' || ch == '~';
			}

			// 验证key 和 value 是否合法
			static void validate(const Atring& key, const Atring& value)
			{
				if (key.empty())
					ValueError("HTTP header name cannot be empty.");
				for (const AChar& ch : key)
					if (!valid_name_char(ch.ord()))
						ValueError(vstr("Invalid HTTP header name: ") + cstr(key));
				for (const AChar& ch : value)
					if (ch == '\r' || ch == '\n')
						ValueError(vstr("HTTP header value contains CR or LF: ") + cstr(key));
			}

			c_size find_index(const Atring& key) const
			{
				auto fn = [&key](const Entry& entry) { return key_equal(key, entry.first); };
				return entries_.index_if(fn, 0);
			}
		};
	}
}

#endif // AYR_NET_HTTP_HEADERS_HPP
