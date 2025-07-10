#ifndef AYR_JSON_PARSE_HPP
#define AYR_JSON_PARSE_HPP

#include "JsonValue.h"

namespace ayr
{
	namespace json
	{
		class JsonParser : Object<JsonParser>
		{
		public:
			JsonParser() {}

			Json operator()(Atring json_str) const
			{
				json_str = json_str.strip();
				return parse_obj(json_str);
			}
		private:
			// 解析 str
			Json _parse_str(JsonStr& json_str) const
			{
				for (c_size i = 1, n = json_str.size(); i < n; i++)
					if (json_str.atchar(i) == '"' && json_str.atchar(i - 1) != '\\')
					{
						JsonStr str_part = json_str.slice(1, i);
						json_str = json_str.slice(i + 1);
						return Json(std::move(str_part));
					}

				RuntimeError(std::format("invalid str parse: {}", json_str));
				return None<Json>;
			}

			// 解析 number, bool, null
			Json _parse_simple(JsonStr& json_str) const
			{
				if (json_str.startswith("null")) // null类型
				{
					json_str = json_str.slice(4);
					return Json();
				}
				else if (json_str.startswith("true"))
				{
					json_str = json_str.slice(4);
					return Json(true);
				}
				else if (json_str.startswith("false"))
				{
					json_str = json_str.slice(5);
					return Json(false);
				}
				else if (json_str.atchar(0) == '-' || json_str[0].isdigit()) // number类型
				{
					bool float_flag = false;
					c_size r = 1;
					while (r < json_str.size())
					{
						if (json_str[r].isdigit())
						{
							++r;
							continue;
						}

						if (json_str.atchar(r) == '.' && !float_flag)
						{
							float_flag = true;
							++r;
							continue;
						}
						break;
					}

					auto num_part = json_str.slice(0, r);
					json_str = json_str.slice(r);

					return ifelse(float_flag, make_float(num_part.to_double()), make_int(num_part.to_int()));
				}

				ValueError(std::format("invalid simple parse: {}", json_str));
				return None<Json>;
			}

			// 返回一个可以被实际解析为Json对象的字符串
			Json parse_obj(JsonStr& json_str) const
			{
				if (json_str.atchar(0) == '{')  // dict类型
				{
					return _parse_dict(json_str);
				}
				else if (json_str.atchar(0) == '[')  // array类型)
				{
					return _parse_array(json_str);
				}
				else if (json_str.atchar(0) == '"')  // str类型
					return _parse_str(json_str);
				else
					return _parse_simple(json_str);
			}

			// 解析array
			Json _parse_array(JsonStr& json_str) const
			{
				JsonArray arr;
				// 去掉 [
				c_size pos = first_non_space(json_str, 1);
				json_str = json_str.slice(pos);

				if (json_str.atchar(0) == ']')
				{
					json_str = json_str.slice(1);
					return Json(std::move(arr));
				}

				while (json_str.atchar(0) != ']')
				{
					arr.append(parse_obj(json_str));

					pos = first_non_space(json_str);
					if (json_str.atchar(pos) == ']')
					{
						json_str = json_str.slice(first_non_space(json_str, pos + 1));
						break;
					}
					else if (json_str.atchar(pos) == ',')
					{
						json_str = json_str.slice(first_non_space(json_str, pos + 1));
						continue;
					}

					ValueError(std::format("invalid array parse: {}", json_str));
					return None<Json>;
				}

				return Json(std::move(arr));
			}


			// 解析dict
			Json _parse_dict(JsonStr& json_str) const
			{
				JsonDict dict;
				// 去掉 {
				c_size pos = first_non_space(json_str, 1);
				json_str = json_str.slice(pos);

				// dict为空
				if (json_str.atchar(0) == '}')
				{
					json_str = json_str.slice(1);
					return Json(std::move(dict));
				}

				// dict非空
				while (json_str.atchar(0) != '}')
				{
					// 解析到key
					Json key = parse_obj(json_str);
					if (!key.is_str())
					{
						ValueError(std::format("invalid dict key parse: {}", json_str));
						return None<Json>;
					}

					// 找到 ':'
					pos = first_non_space(json_str);
					if (json_str.atchar(pos) != ':')
					{
						ValueError(std::format("invalid dict parse: {}", json_str));
						return None<Json>;
					}

					// 解析value
					json_str = json_str.slice(first_non_space(json_str, pos + 1));
					dict[key.as_str()] = parse_obj(json_str);

					// 找到 ','或 '}'
					pos = first_non_space(json_str);
					if (json_str.atchar(pos) == '}')
					{
						json_str = json_str.slice(first_non_space(json_str, pos + 1));
						break;
					}
					else if (json_str.atchar(pos) == ',')
					{
						json_str = json_str.slice(first_non_space(json_str, pos + 1));
						continue;
					}

					ValueError(std::format("invalid dict parse: {}", json_str));
					return None<Json>;
				}

				return Json(std::move(dict));
			}

		private:
			// 从pos开始第一个不是空白符的位置
			c_size first_non_space(const JsonStr& json_str, c_size pos = 0) const
			{
				while (pos < json_str.size() && json_str[pos].isspace()) ++pos;
				return pos;
			}
		};
	}
}

#endif