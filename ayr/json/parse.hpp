#ifndef AYR_JSON_PARSE_HPP
#define AYR_JSON_PARSE_HPP

#include "json_obj.hpp"


namespace ayr
{
	namespace json
	{
		// 解析字符串
		def _parse_str(JsonType::JsonStr& json_str)->Json;
		def _parse_num(JsonType::JsonStr& json_str)->Json;
		def _parse_bool(JsonType::JsonStr& json_str)->Json;
		def _parse_null(JsonType::JsonStr& json_str)->Json;
		def _parse_simple(JsonType::JsonStr& json_str)->Json;
		def _parse_array(JsonType::JsonStr& json_str)->Json;
		def _parse_dict(JsonType::JsonStr& json_str)->Json;

		// 返回一个可以被实际解析为Json对象的字符串
		def parse(JsonType::JsonStr& json_str) -> Json
		{
			json_str = json_str.strip();

			if (json_str[0] == CodePoint('{'))  // dict类型
			{
				JsonType::JsonStr match = json_str.match('{', '}');
				return _parse_dict(match);
			}
			else if (json_str[0] == CodePoint('['))  // array类型
			{
				JsonType::JsonStr match = json_str.match('[', ']');
				return _parse_array(match);
			}
			else if (json_str[0] == CodePoint('"'))  // str类型
				return _parse_str(json_str);
			else
				return _parse_simple(json_str);
		}

		def parse(JsonType::JsonStr&& json_str) { return parse(json_str); }

		// 解析 str
		def _parse_str(JsonType::JsonStr& json_str) -> Json
		{
			return Json(json_str.slice(1, -1));
		}


		// 返回解析为num对象的字符串
		def _parse_num(JsonType::JsonStr& json_str) -> Json
		{
			bool float_flag = false;
			for (auto&& c : json_str)
				if (!c.isdigit())
					if (c == CodePoint('.') && !float_flag)
						float_flag = true;
					else
						ValueError(std::format("invalid number parse: {}", json_str));

			CString _cstr = cstr(json_str);
			if (float_flag)
				return make_float(atof(_cstr));
			else
				return make_int(atoll(_cstr));
		}


		def _parse_bool(JsonType::JsonStr& json_str) -> Json
		{
			if (json_str == "true"as)
				return Json(true);
			else if (json_str == "false"as)
				return Json(false);
			else
				ValueError(std::format("invalid bool parse: {}", json_str));
		}


		def _parse_null(JsonType::JsonStr& json_str) -> Json
		{
			if (json_str == JsonType::JsonStr("null", 4))
				return Json(JsonType::JsonNull());
			else
				ValueError(std::format("invalid null parse: {}", json_str));
		}


		def _parse_simple(JsonType::JsonStr& json_str) -> Json
		{
			if (json_str[0].isdigit())  // number类型
				return _parse_num(json_str);
			else if (json_str[0] == CodePoint('n')) // null类型
				return _parse_null(json_str);
			else if (json_str[0] == CodePoint('t') || json_str[0] == CodePoint('f')) // bool类型
				return _parse_bool(json_str);
			else
				ValueError(std::format("invalid simple parse: {}", json_str));
		}


		def parse_first_object(JsonType::JsonStr& json_str, CodePoint stop_sign) -> std::pair<Json, JsonType::JsonStr>
		{
			json_str = json_str.strip();
			if (json_str.size() == 0)
				ValueError("json_str is empty");

			JsonType::JsonStr match;
			if (json_str[0] == CodePoint('['))
			{
				match = json_str.match('[', ']');
			}
			else if (json_str[0] == CodePoint('{'))
			{
				match = json_str.match('{', '}');
			}
			else if (json_str[0] == CodePoint('"'))
			{
				match = json_str.slice(0, json_str.slice(1).find('"') + 2);
			}
			else
			{
				c_size stop_sign_idx = json_str.find(stop_sign);
				if (stop_sign_idx == -1)
					match = json_str;
				else
					match = json_str.slice(0, stop_sign_idx);
			}

			JsonType::JsonStr ret_str = json_str.slice(match.size()).strip();

			if (ret_str.size())
			{
				if (ret_str[0] != stop_sign)
					ValueError(std::format("stop_sign '{}' not found", stop_sign));

				ret_str = ret_str.slice(1).strip();
			}

			return { parse(match), ret_str };
		}


		// 解析array
		def _parse_array(JsonType::JsonStr& json_str) -> Json
		{
			JsonType::JsonArray arr;
			// 去掉 []
			json_str = json_str.slice(1, -1).strip();
			while (json_str.size())
			{
				auto [item, _json_str] = parse_first_object(json_str, ',');
				arr.append(item);
				json_str = _json_str;
			}

			return Json(std::move(arr));
		}


		// 解析dict
		def _parse_dict(JsonType::JsonStr& json_str) -> Json
		{
			JsonType::JsonDict dict;
			json_str = json_str.slice(1, -1).strip();

			while (json_str.size())
			{
				auto [key, _json_str1] = parse_first_object(json_str, ':');
				json_str = _json_str1;

				auto [value, _json_str2] = parse_first_object(json_str, ',');
				json_str = _json_str2;

				dict[key] = std::move(value);
			}

			return Json(std::move(dict));
		}
	}
}

#endif