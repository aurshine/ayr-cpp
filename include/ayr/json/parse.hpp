#ifndef AYR_JSON_PARSE_HPP
#define AYR_JSON_PARSE_HPP

#include "JsonValue.h"

namespace ayr
{
	namespace json
	{
		// 解析字符串
		def _parse_str(JsonStr& json_str) -> Json;
		def _parse_num(JsonStr& json_str) -> Json;
		def _parse_bool(JsonStr& json_str) -> Json;
		def _parse_null(JsonStr& json_str) -> Json;
		def _parse_simple(JsonStr& json_str) -> Json;
		def _parse_array(JsonStr& json_str) -> Json;
		def _parse_dict(JsonStr& json_str) -> Json;

		// 返回一个可以被实际解析为Json对象的字符串
		def parse(JsonStr& json_str) -> Json
		{
			json_str = json_str.strip();

			if (json_str[0] == "{")  // dict类型
			{
				JsonStr match = json_str.match("{", "}");
				return _parse_dict(match);
			}
			else if (json_str[0] == "[")  // array类型
			{
				JsonStr match = json_str.match("[", "]");
				return _parse_array(match);
			}
			else if (json_str[0] == "\"")  // str类型
				return _parse_str(json_str);
			else
				return _parse_simple(json_str);
		}

		def parse(JsonStr&& json_str) { return parse(json_str); }

		// 解析 str
		def _parse_str(JsonStr& json_str) -> Json { return Json(json_str.slice(1, -1)); }


		// 返回解析为num对象的字符串
		def _parse_num(JsonStr& json_str) -> Json
		{
			bool float_flag = false;
			for (auto&& c : json_str)
				if (!c.isdigit())
					if (c == "." && !float_flag)
						float_flag = true;
					else
						ValueError(std::format("invalid number parse: {}", json_str));

			CString _cstr = cstr(json_str);
			if (float_flag)
				return make_float(atof(_cstr));
			else
				return make_int(atoll(_cstr));
		}


		def _parse_bool(JsonStr& json_str) -> Json
		{
			if (json_str == "true"as) return Json(true);
			if (json_str == "false"as) return Json(false);

			ValueError(std::format("invalid bool parse: {}", json_str));
			return None<Json>;
		}


		def _parse_null(JsonStr& json_str) -> Json
		{
			if (json_str == JsonStr("null", 4))
				return Json();

			ValueError(std::format("invalid null parse: {}", json_str));
			return None<Json>;
		}


		def _parse_simple(JsonStr& json_str) -> Json
		{
			if (json_str[0].isdigit())  // number类型
				return _parse_num(json_str);
			else if (json_str[0] == "n") // null类型
				return _parse_null(json_str);
			else if (json_str[0] == "t" || json_str[0] == "f") // bool类型
				return _parse_bool(json_str);

			ValueError(std::format("invalid simple parse: {}", json_str));
			return None<Json>;
		}


		def parse_first_object(JsonStr& json_str, const JsonStr& stop_sign) -> std::pair<Json, JsonStr>
		{
			json_str = json_str.strip();
			if (json_str.size() == 0)
				ValueError("json_str is empty");

			JsonStr match;
			if (json_str[0] == "[")
			{
				match = json_str.match("[", "]");
			}
			else if (json_str[0] == "{")
			{
				match = json_str.match("{", "}");
			}
			else if (json_str[0] == "\"")
			{
				match = json_str.slice(0, json_str.find("\"", 1) + 1);
			}
			else
			{
				c_size stop_sign_idx = json_str.find(stop_sign);
				if (stop_sign_idx == -1)
					match = json_str;
				else
					match = json_str.slice(0, stop_sign_idx);
			}

			JsonStr other_str = json_str.slice(match.size()).strip();

			// 还有剩余
			if (other_str.size())
				if (!other_str.startswith(stop_sign))
					ValueError(std::format("stop_sign '{}' not found, other_str: {}", stop_sign, other_str));
				else
					other_str = other_str.slice(stop_sign.size()).strip();

			return { parse(match), other_str };
		}


		// 解析array
		def _parse_array(JsonStr& json_str) -> Json
		{
			JsonArray arr;
			// 去掉 []
			json_str = json_str.slice(1, -1).strip();
			while (json_str.size())
			{
				auto [item, _json_str] = parse_first_object(json_str, ",");
				arr.append(item);
				json_str = _json_str;
			}

			return Json(std::move(arr));
		}


		// 解析dict
		def _parse_dict(JsonStr& json_str) -> Json
		{
			JsonDict dict;
			json_str = json_str.slice(1, -1).strip();

			while (json_str.size())
			{
				auto [key, _json_str1] = parse_first_object(json_str, ":");
				json_str = _json_str1;

				auto [value, _json_str2] = parse_first_object(json_str, ",");
				json_str = _json_str2;

				dict[key.as_str()] = std::move(value);
			}

			return Json(std::move(dict));
		}
	}
}

#endif