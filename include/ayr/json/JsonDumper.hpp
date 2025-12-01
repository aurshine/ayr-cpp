#ifndef AYR_JSON_JSONDUMPER_HPP
#define AYR_JSON_JSONDUMPER_HPP

#include "JsonValue.hpp"

namespace ayr
{
	namespace json
	{
		class JsonDumper
		{
			Buffer& buffer;

			// 当前缩进级别
			int depth;

			// 每行最多元素数，超过则换行
			static constexpr int MAX_ELEMENTS_INLINE = 10;

		public:
			JsonDumper(Buffer& buffer_) : buffer(buffer_), depth(0) {}

			void operator()(const JsonNull& obj) { buffer << "null"; }

			void operator()(const JsonInt& obj) { buffer << obj; }

			void operator()(const JsonFloat& obj) { buffer << obj; }

			void operator()(const JsonBool& obj) { buffer << obj; }

			void operator()(const JsonStr& obj) { buffer << "\"" << obj << "\""; }

			template<JsonLikeConcept T>
			void operator()(const DynArray<T>& obj)
			{
				bool auto_line = needs_auto_line(obj);

				buffer << "[";
				++depth;
				// 已经输出第一个元素的标志
				bool flag = false;
				for (auto& item : obj)
				{
					if (flag) buffer << ", ";
					if (auto_line) newline();

					flag = true;
					this->operator()(item);
				}

				--depth;
				if (auto_line) newline();
				buffer << "]";
			}

			template<JsonLikeConcept T>
			void operator()(const Dict<JsonStr, T>& obj)
			{
				bool auto_line = needs_auto_line(obj);

				buffer << "{";
				++depth;

				// 已经输出第一个元素的标志
				bool flag = false;
				for (auto& [k, v] : obj.items())
				{
					if (flag) buffer << ", ";
					if (auto_line) newline();

					flag = true;
					this->operator()(k);
					buffer << ": ";
					this->operator()(v);
				}

				--depth;
				if (auto_line) newline();
				buffer << "}";
			}

			void operator()(const Json& obj)
			{
				obj.visit([&](auto&& item_) {
					this->operator()(item_);
					});
			}
		private:
			// 输出换行和缩进
			void newline()
			{
				buffer.expand_util(1 + depth * 4);
				buffer.append_bytes("\n", 1, 1);
				buffer.append_bytes(" ", 1, depth * 4);
			}

			// 包含数据结构或元素过多时, 每个元素单独一行
			template<JsonLikeConcept T>
			bool needs_auto_line(const DynArray<T>& arr) const
			{
				if (arr.size() > MAX_ELEMENTS_INLINE)
					return true;
				return any(arr, [](auto& item) {
					return !sample_type(item);
					});
			}

			// 包含数据结构或元素过多时, 每个元素单独一行
			template<JsonLikeConcept T>
			bool needs_auto_line(const Dict<JsonStr, T>& dict) const
			{
				if (dict.size() > MAX_ELEMENTS_INLINE)
					return true;
				return any(dict.values(), [](auto& item) {
					return !sample_type(item);
					});
			}
		};

		// 将JsonLike对象转储为JSON字符串 - CString版本
		template <JsonLikeConcept T>
		CString dump(const T& obj)
		{
			Buffer buffer(512);
			JsonDumper dumper(buffer);
			dumper(obj);
			return from_buffer(std::move(buffer));
		}

		// 将JsonLike对象转储为JSON字符串 - Atring版本
		template<JsonLikeConcept T>
		Atring dumps(const T& obj)
		{
			Buffer buffer(512);
			JsonDumper dumper(buffer);
			dumper(obj);
			return Atring::from(vstr(buffer.peek(), buffer.readable_size()));
		}

		// 将JsonLike对象转储为JSON字符串 - Atring版本，指定编码
		template<UniCodec Codec, JsonLikeConcept T>
		Atring dumps(const T& obj)
		{
			Buffer buffer(512);
			JsonDumper dumper(buffer);
			dumper(obj);
			return Atring::from<Codec>(vstr(buffer.peek(), buffer.readable_size()));
		}

		void Json::__repr__(Buffer& buffer) const
		{
			JsonDumper dumper(buffer);
			dumper(*this);
		}
	}
}

#endif // AYR_JSON_JSONDUMPER_HPP