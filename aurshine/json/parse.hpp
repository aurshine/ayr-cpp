#include <json/json_obj.hpp>
#include <law/AString.hpp>


namespace ayr
{
	namespace json
	{
		// 解析字符串
		Json parse(const Astring& json_str, size_t& start);


		// 从 start 开始跳过所有空白符，即返回第一个非空白符的下标或 json_str 的长度
		size_t jump_blank(const Astring& json_str, size_t start)
		{
			while (start < json_str.size() && isspace(json_str[start]))	++start;
			return start;
		}


		// 从 start 开始跳到第一个空白符, 即返回第一个空白符的下标或 json_str 的长度
		size_t jump2blank(const Astring& json_str, size_t start)
		{
			while (start < json_str.size() && !isspace(start)) ++start;
			return start;
		}

		// 解析 str
		Json parse_str(const Astring& json_str, size_t& start)
		{
			start = jump_blank(json_str, start);

			error_assert(json_str[start] == '"', "error parse for object not String");

			size_t end = ++start;
			while (end < json_str.size() && json_str[end] != '"') ++end;

			error_assert(json_str[end] == '"', "error parse for object not String");

			Json ret = Json(json_str.slice(start, end));

			start = end + 1;
			return ret;
		}

		Json parse_array(const Astring& json_str, size_t& start)
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

		Json parse_dict(const Astring& json_str, size_t& start)
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

		Json parse_num(const Astring& json_str, size_t& start)
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

		Json parse(const Astring& json_str, size_t& start)
		{
			start = jump_blank(json_str, start);

			if (json_str[start] == '{')
				return parse_dict(json_str, start);
			else if (json_str[start] == '[')
				return parse_array(json_str, start);
			else if (json_str[start] == '"')
			{
				return parse_str(json_str, start);
			}
			else if (isdigit(json_str[start]))
				return parse_num(json_str, start);
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
					error_assert(false, str);
			}

		}

		Json parse(const Astring& json_str)
		{
			size_t start = 0;
			return parse(json_str, start);
		}
	}
}