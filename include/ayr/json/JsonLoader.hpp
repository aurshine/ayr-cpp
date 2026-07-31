#ifndef AYR_JSON_JSONLOADER_HPP
#define AYR_JSON_JSONLOADER_HPP

#include "JsonValue.hpp"

namespace ayr
{
	namespace json
	{
		class JsonLoader
		{
			// 需要解析的json字符串
			Atring json_str_;

			// 当前解析位置，[pos_, end_)为未解析部分
			c_size pos_, end_;

			// 解析深度，超过一定深度抛出异常，防止恶意输入导致栈溢出
			c_size depth_ = 0;
		public:
			inline static c_size MAX_DEPTH = DEFAULT_MAX_DEPTH;

			JsonLoader(const Atring& json_str): json_str_(json_str.strip()), pos_(0), end_(json_str_.size()) {}

			/*
			* @brief 解析json字符串
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象和剩余未解析的字符串
			*/
			std::pair<Json, Atring> operator()()
			{
				Json json_obj = parse_obj();
				return { std::move(json_obj), json_str_.slice(pos_) };
			}
		private:
			/*
			* @brief 解析 number, bool, null
			*
			* @param json_str 待解析的json字符串
			*
			* @return 解析得到的Json对象
			*/
			Json _parse_simple()
			{
				if (startswith("null"as)) // null类型
				{
					pos_ += 4;
					return Json();
				}
				else if (startswith("true"as)) // bool类型
				{
					pos_ += 4;
					return Json(true);
				}
				else if (startswith("false"as)) // bool类型
				{
					pos_ += 5;
					return Json(false);
				}
				else if (startswith("-"as) || json_str_.at(pos_).isdigit()) // number类型
				{
					bool negative = startswith("-"as);
					
					auto [num_int, int_remain_str] = remain_str().toint();
					if (int_remain_str.startswith("."as))
					{
						auto [num_float, float_remain_str] = int_remain_str.tofloat();
						pos_ = end_ - float_remain_str.size();
						return floating(ifelse(negative, num_int - num_float, num_int + num_float));
					}

					pos_ = end_ - int_remain_str.size();
					return integer(num_int);
				}
				ValueError(ayr::format("invalid simple parse: {}", first_char()));
				return None;
			}

			/*
			* @brief 解析 str
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象
			*/
			Json _parse_str()
			{
				// 去掉 "
				++pos_;
				DynArray<JsonStr> ds;
				c_size l = pos_;
				while (pos_ < end_)
				{
					AChar ch = json_str_.at(pos_ ++);
					if (ch.ord() < 0x20)
						JsonValueError("unescaped control character in json string");
					
					if (ch == '"')
					{
						if (ds.empty())
							return json_str_.slice(l, pos_ - 1);
						ds.append(json_str_.slice(l, pos_ - 1));
						return Json(Atring::ajoin(ds));
					}

					if (ch == '\\' && pos_ < end_)
					{
						ds.append(json_str_.slice(l, pos_ - 1));
						ds.append(parse_escape());
						l = pos_;
					}
				}

				JsonValueError("unterminated json string");
				return None;
			}

			/*
			* @brief 解析 array
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象
			*/
			Json _parse_array()
			{
				JsonArray arr;
				// 去掉 [
				first_non_space(1);
				
				while (first_char() != ']')
				{
					arr.append(parse_obj());

					first_non_space();
					if (first_char() == ']')
						break;
					else if (first_char() == ',')
						first_non_space(1); // 去掉 ,
					else
						JsonValueError(ayr::format("invalid array parse: {}", remain_str()));
				}
				
				// 去掉 ]
				++ pos_;
				return Json(std::move(arr));
			}

			/*
			* @brief 解析 dict
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象
			*/
			Json _parse_dict()
			{
				JsonDict dict;
				// 去掉 {
				first_non_space(1);

				// dict非空
				while (first_char() != '}')
				{
					// 解析到key
					Json key = parse_obj();
					if (!key.is<JsonStr>())
						JsonValueError(ayr::format("invalid dict key parse: {}", key));
					
					// 找到 ':'
					first_non_space();
					if (first_char() != ':')
						JsonValueError(ayr::format("invalid dict parse: {}, expect ':'", first_char()));

					// 去掉 ':'
					first_non_space(1);
					// 解析value
					dict.insert(std::move(key.as<JsonStr>()), parse_obj());

					// 找到 ','或 '}'
					first_non_space();
					if (first_char() == '}')
						break;
					else if (first_char() == ',')
						first_non_space(1);
					else
						JsonValueError(ayr::format("invalid dict parse: {}", remain_str()));
				}

				// 去掉 }
				++pos_;
				return Json(std::move(dict));
			}

			/*
			* @brief 解析json对象
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象
			*/
			Json parse_obj()
			{
				enter_depth();
				exitask([&] { --depth_; });
				
				AChar ch = first_char();
				if (ch == '{')  // dict类型
					return _parse_dict();
				else if (ch == '[')  // array类型)
					return _parse_array();
				else if (ch == '"')  // str类型
					return _parse_str();
				else
					return _parse_simple();
			}

			// 从pos_ + offset 开始第一个不是空白符的位置
			void first_non_space(c_size offset = 0)
			{
				pos_ += offset;
				while (pos_ < end_ && first_char().isspace()) ++pos_;
			}

			bool startswith(const Atring& prefix)
			{
				if (pos_ + prefix.size() > end_)
					return false;
				for (c_size i = 0, n = prefix.size(); i < n; ++ i)
					if (json_str_.at(pos_ + i) != prefix.at(i))
						return false;
				return true;
			}

			// 获取剩余字符串
			Atring remain_str() const { return json_str_.slice(pos_); }
			
			// 获取剩余字符串的第一个字符
			AChar first_char() const { return json_str_.at(pos_); }

			// 进入一层容器并检查解析深度
			void enter_depth()
			{
				if (++ depth_ > MAX_DEPTH)
					JsonValueError(ayr::format("exceed max depth: {}", MAX_DEPTH));
			}

			// 解析JSON字符串中的转义字符并写入缓冲区
			AChar parse_escape()
			{
				AChar escaped = json_str_.at(pos_ ++);
				if (escaped == '"' || escaped == '\\' || escaped == '/')
					return escaped;
				else if (escaped == 'b')
					return '\b';
				else if (escaped == 'f')	
					return '\f';
				else if (escaped == 'n')
					return '\n';
				else if (escaped == 'r')
					return '\r';
				else if (escaped == 't')
					return '\t';
				else
					JsonValueError(ayr::format("invalid escape character: {}", escaped));
			}
		};

		/*
		* @brief 解析json字符串
		* 
		* @param json_str 待解析的json字符串
		* 
		* @return 解析得到的Json对象和剩余未解析的字符串
		*/
		std::pair<Json, Atring> loads_prefix(const Atring& json_str) { return JsonLoader(json_str)(); }

		/*
		* @brief 解析json字符串
		*
		* @param json_str 待解析的json字符串
		*
		* @return 解析得到的Json对象
		*/
		Json loads(const Atring& json_str)
		{
			auto [json_obj, remain_str] = loads_prefix(json_str);
			if (!remain_str.empty())
				JsonValueError(ayr::format("load remaining, {}", remain_str));

			return json_obj;
		}
	}
}

#endif // AYR_JSON_JSONLOADER_HPP