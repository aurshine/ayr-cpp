#ifndef AYR_JSON_PARSE_HPP
#define AYR_JSON_PARSE_HPP

#include "json_obj.hpp"


namespace ayr
{
	namespace json
	{
		// 解析字符串
		def __parse_str(JsonType::JsonStr& json_str)->Json;
		def __parse_num(JsonType::JsonStr& json_str)->Json;
		def __parse_bool(JsonType::JsonStr& json_str)->Json;
		def __parse_null(JsonType::JsonStr& json_str)->Json;
		def __parse_simple(JsonType::JsonStr& json_str)->Json;
		def __parse_array(JsonType::JsonStr& json_str)->Json;
		def __parse_dict(JsonType::JsonStr& json_str)->Json;


		// 返回一个可以被实际解析为Json对象的字符串
		def parse(JsonType::JsonStr& json_str) -> Json
		{
			json_str.strip_();

			if (json_str[0] == '{')  // dict类型
			{
				JsonType::JsonStr match = json_str.match('{', '}');
				return __parse_dict(match);
			}
			else if (json_str[0] == '[')  // array类型
			{
				JsonType::JsonStr match = json_str.match('[', ']');
				return __parse_array(match);
			}
			else if (json_str[0] == '"')  // str类型
				return __parse_str(json_str);
			else
				return __parse_simple(json_str);
		}


		// 解析 str
		def __parse_str(JsonType::JsonStr& json_str) -> Json
		{
			return Json(json_str.slice(1, -1));
		}


		// 返回解析为num对象的字符串
		def __parse_num(JsonType::JsonStr& json_str) -> Json
		{
			bool float_flag = false;
			for (auto c : json_str)
				if (!isdigit(c))
					if (c == '.' && !float_flag)
						float_flag = true;
					else
						ValueError(json_str.__str__());

			Json ret = float_flag ? Json(make_float(atof(json_str.__str__())))
				: Json(make_int(atoll(json_str.__str__())));

			return ret;
		}


		def __parse_bool(JsonType::JsonStr& json_str) -> Json
		{
			if (json_str == "true"as)
				return Json(true);
			else if (json_str == "false"as)
				return Json(false);
			else
				ValueError(stdstr(json_str));
		}


		def __parse_null(JsonType::JsonStr& json_str) -> Json
		{
			if (json_str == JsonType::JsonStr("null", 4))
				return Json(JsonType::JsonNull());
			else
				ValueError(stdstr(json_str));
		}


		def __parse_simple(JsonType::JsonStr& json_str) -> Json
		{
			if (isdigit(json_str[0]))  // number类型
				return __parse_num(json_str);
			else if (json_str[0] == 'n') // null类型
				return __parse_null(json_str);
			else if (json_str[0] == 't' || json_str[0] == 'f') // bool类型
				return __parse_bool(json_str);
			else
				ValueError(stdstr(json_str));
		}


		def parse_first_object(JsonType::JsonStr& json_str, char stop_sign) -> std::pair<Json, JsonType::JsonStr>
		{
			json_str.strip_();
			error_assert(json_str.size() == 0, "json_str is empty");

			JsonType::JsonStr match;
			if (json_str[0] == '[')
			{
				match = json_str.match('[', ']');
			}
			else if (json_str[0] == '{')
			{
				match = json_str.match('{', '}');
			}
			else if (json_str[0] == '"')
			{
				match = json_str.slice(0, json_str.find('"', 1) + 1);
			}
			else
			{
				c_size stop_sign_idx = json_str.find(stop_sign, 0);
				if (stop_sign_idx == -1)
					match = json_str;
				else
					match = json_str.slice(0, stop_sign_idx);
			}

			JsonType::JsonStr ret_str = json_str.slice(match.size()).strip_();

			if (ret_str.size())
			{
				if (ret_str[0] != stop_sign)
					RuntimeError(std::format("stop_sign '{}' not found", stop_sign));

				ret_str.slice_(1).strip_();
			}

			return { parse(match), ret_str };
		}


		// 解析array
		def __parse_array(JsonType::JsonStr& json_str) -> Json
		{
			JsonType::JsonArray arr;
			// 去掉 []
			json_str.slice_(1, -1);
			json_str.strip_();

			while (json_str.size())
			{
				auto [item, _json_str] = parse_first_object(json_str, ',');
				arr.append(item);
				json_str = _json_str;
			}

			return Json(std::move(arr));
		}


		// 解析dict
		def __parse_dict(JsonType::JsonStr& json_str) -> Json
		{
			JsonType::JsonDict dict;
			json_str.slice_(1, -1);
			json_str.strip_();

			while (json_str.size())
			{
				auto [key, _json_str1] = parse_first_object(json_str, ':');
				json_str = _json_str1;

				auto [value, _json_str2] = parse_first_object(json_str, ',');
				json_str = _json_str2;

				dict[key.transform<JsonType::JsonStr>()] = std::move(value);
			}

			return Json(std::move(dict));
		}
	}
}

#endif