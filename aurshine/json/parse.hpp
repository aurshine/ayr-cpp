#include <json/json_obj.hpp>
#include <law/SubString.hpp>


namespace ayr
{
	namespace json
	{
		// 解析字符串
		Json parse(const Str& json_str, size_t& start);


		// 从 start 开始跳过所有空白符，即返回第一个非空白符的下标或 json_str 的长度
		size_t jump_blank(const Str& json_str, size_t start)
		{
			while (start < json_str.size() && isspace(json_str[start]))	++start;
			return start;
		}


		// 从 start 开始跳到第一个空白符, 即返回第一个空白符的下标或 json_str 的长度
		size_t jump2blank(const Str& json_str, size_t start)
		{
			while (start < json_str.size() && !isspace(start)) ++start;
			return start;
		}

		// 解析 str
		Json parse_str(const Str& json_str)
		{
			c_size start = json_str.find('"');
			error_assert(start != -1, "parse str error");
			c_size end = json_str.find('"', start + 1);

			Json ret = Json(json_str.slice(start, end).strme());

			start = end + 1;
			return ret;
		}

		Json parse_array(const Str& json_str)
		{
			start = jump_blank(json_str, start);
			error_assert(json_str[start] == '[', "error parse for object not Array by [");
			JsonType::JsonArray arr;

			++start;
			while (json_str[start] != ']')
			{
				start = jump_blank(json_str, start);

				Json j = parse(json_str, start);
				arr.append(j);
				
				start = jump_blank(json_str, start);

				if (json_str[start] == ',')
					++start;
				else if (json_str[start] == ']')
					break;
				else
				{
					error_assert(false, "error parse for object not Array by , ]");
				}

			}

			++start;
			return Json(std::move(arr));
		}

		Json parse_dict(const Str& json_str)
		{
			start = jump_blank(json_str, start);
			
			error_assert(json_str[start] == '{', "error parse for object not Dict");
			typename JsonType::JsonDict dict;

			int end = ++start;
			while (json_str[start] != '}')
			{
				start = jump_blank(json_str, start);
				error_assert(json_str[start] == '"', "");

				Json key = parse_str(json_str, start);
				start = jump_blank(json_str, start);
				error_assert(json_str[start] == ':', "error parse for object not Dict");

				dict[key.transform<typename JsonType::JsonStr>()] = parse(json_str, ++start);

				start = jump_blank(json_str, start);
				if (json_str[start] == ',')
					++start;
				else if (json_str[start] == '}')
					break;
				else
				{
					error_assert(false, "error parse for object not Dict by , }");
				}
			}

			++start;
			return Json(std::move(dict));
		}

		Json parse_num(const Str& json_str)
		{
			start = jump_blank(json_str, start);
			error_assert(isdigit(json_str[start]), "error parse for object not Number");

			size_t end = start;
			bool float_flag = false;
			while (end < json_str.size() &&
				(isdigit(json_str[end]) || (json_str[end] == '.' && !float_flag)))
			{
				if (json_str[end] == '.')
					float_flag = true;
				++end;
			}

			Json ret = float_flag ? Json(make_float(atof(json_str.slice(start, end).__str__().str))) 
									: Json(make_int(atoll(json_str.slice(start, end).__str__().str)));


			start = end;
			return ret;
		}

		Json _parse(const Str& _json_str)
		{
			Str json_str = _json_str.strip();

			if (json_str[0] == '{')
				return parse_dict(json_str.match("{", "}"));
			else if (json_str[0] == '[')
				return parse_array(json_str);
			else if (json_str[0] == '"')
			{
				return parse_str(json_str);
			}
			else if (isdigit(json_str[0]))
				return parse_num(json_str);
			else
			{
				Astring str = json_str.slice(start, start + 4);
				start += 4;
				if (str == "null")
					return Json();
				else if (str == "true")
					return Json(true);
				else if (str == "false")
					return Json(false);
				else
					ValueError(str);
			}
		}

		Json parse(const Astring& json_str)
		{
			return _parse(json_str.subme());
		}
	}
}