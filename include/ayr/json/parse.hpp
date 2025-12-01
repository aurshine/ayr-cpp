#ifndef AYR_JSON_PARSE_HPP
#define AYR_JSON_PARSE_HPP

#include "JsonValue.h"

namespace ayr
{
	namespace json
	{
		class JsonParser
		{
			// 需要解析的json字符串
			Atring json_str_;

			// 当前解析位置，[pos_, ]为未解析部分
			c_size pos_;
		public:
			JsonParser(const Atring& json_str): json_str_(json_str.strip()), pos_(0) {}

			/*
			* @brief 解析json字符串
			* 
			* @param json_str 待解析的json字符串
			* 
			* @return 解析得到的Json对象和解析的字符串的长度
			*/
			std::pair<Json, c_size> operator()()
			{
				Json json_obj = parse_obj();
				return { std::move(json_obj), pos_};
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
				Atring json_str = remain_str();
				if (json_str.startswith("null"as)) // null类型
				{
					pos_ += 4;
					return Json();
				}
				else if (json_str.startswith("true"as)) // bool类型
				{
					pos_ += 4;
					return Json(true);
				}
				else if (json_str.startswith("false"as)) // bool类型
				{
					pos_ += 5;
					return Json(false);
				}
				else if (json_str.at(0) == '-' ||
					json_str.at(0) == '+' ||
					json_str[0].isdigit()
					) // number类型
				{
					c_size i = json_str.at(0) == '+' || json_str.at(0) == '-';
					while (i < json_str.size() && json_str[i].isdigit()) ++i;

					// float类型
					if (i < json_str.size() && json_str.at(i) == '.')
					{
						auto&& [num, remain_str] = json_str.tofloat();
						pos_ = json_str_.size() - remain_str.size();
						return Json(num);
					}

					// int类型
					auto&& [num, remain_str] = json_str.toint();
					pos_ = json_str_.size() - remain_str.size();
					return Json(num);
				}
				ValueError(std::format("invalid simple parse: {}", json_str.at(0)));
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
				c_size start_pos = ++ pos_;
				c_size n = json_str_.size();
				while (pos_ < n)
				{
					// 遇到未转义的 "
					if (json_str_.at(pos_) == '"' && json_str_.at(pos_ - 1) != '\\')
						return Json(json_str_.slice(start_pos, pos_ ++));
					++pos_;
				}
					
				JsonValueError(std::format("invalid str parse: {}", json_str_.vslice(start_pos)));
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
						JsonValueError(std::format("invalid array parse: {}", remain_str()));
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
					if (!key.is_str())
						JsonValueError(std::format("invalid dict key parse: {}", key));
					
					// 找到 ':'
					first_non_space();
					if (first_char() != ':')
						JsonValueError(std::format("invalid dict parse: {}, expect ':'", first_char()));

					// 去掉 ':'
					first_non_space(1);
					// 解析value
					dict.insert(std::move(key.as_str()), parse_obj());

					// 找到 ','或 '}'
					first_non_space();
					if (first_char() == '}')
						break;
					else if (first_char() == ',')
						first_non_space(1);
					else
						JsonValueError(std::format("invalid dict parse: {}", remain_str()));
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
				if (first_char() == '{')  // dict类型
					return _parse_dict();
				else if (first_char() == '[')  // array类型)
					return _parse_array();
				else if (first_char() == '"')  // str类型
					return _parse_str();
				else
					return _parse_simple();
			}

			// 从pos_ + offset 开始第一个不是空白符的位置
			void first_non_space(c_size offset = 0)
			{
				pos_ += offset;
				while (pos_ < json_str_.size() && first_char().isspace()) ++pos_;
			}

			// 获取剩余字符串
			Atring remain_str() const { return json_str_.vslice(pos_); }
			
			// 获取剩余字符串的第一个字符
			AChar first_char() const { return json_str_.at(pos_); }
	};

		/*
		* @brief 解析json字符串
		* 
		* @param json_str 待解析的json字符串
		* 
		* @return 解析得到的Json对象
		*/
		Json load(const Atring& json_str)
		{
			JsonParser parser(json_str);
			return parser().first;
		}

		/*
		* @brief 解析json字符串
		* 
		* @param json_str 待解析的json字符串
		* 
		* @return 解析得到的Json对象和剩余未解析的字符串
		*/
		std::pair<Json, Atring> loads(const Atring& json_str)
		{
			JsonParser parser(json_str);
			auto [json_obj, pos] = parser();
			return { json_obj, json_str.vslice(pos) };
		}
	}
}

#endif