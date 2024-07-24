#include <json/json_obj.hpp>
#include <law/SubString.hpp>


namespace ayr
{
	namespace json
	{
		// 解析字符串
		def parse(const Str& json_str) -> Json;
		def parse(const Astring& json_str) -> Json;

		// 解析 str
		def __parse_str(const Str& json_str) -> Json
		{
			return Json(json_str.slice(1, -1).strme());
		}

		// 解析array
		def __parse_array(const Str& json_str) -> Json
		{
			JsonType::JsonArray arr;
			// 去掉 []
			json_str = json_str.slice(1, -1);
			Array<Astring> items = json_str.strme().split(",");

			for (auto&& item : items)
				arr.append(parse(item));
				
			return Json(std::move(arr));
		}

		// 解析dict
		def __parse_dict(const Str& json_str) -> Json
		{
			typename JsonType::JsonDict dict;

			auto kv_strs = json_str.strme().split(",");

			return Json(std::move(dict));
		}

		// 返回解析为num对象的字符串
		def __parse_num(const Str& json_str) -> Json
		{
			bool float_flag = false;
			for (auto c : json_str)
				if (!isdigit(c))
					if (c == '.' && !float_flag)
						float_flag = true;
					else
						ValueError(json_str.__str__().str);

			Json ret = float_flag ? Json(make_float(atof(json_str.strme().__str__().str)))
									: Json(make_int(atoll(json_str.strme().__str__().str)));

			return ret;
		}

		// 返回一个可以被实际解析为Json对象的字符串
		def parse(const Str& json_str) -> Json
		{
			json_str.strip_();

			if (json_str[0] == '{')  // object类型
				return __parse_dict(json_str.match('{', '}'));
			else if (json_str[0] == '[')  // array类型
				return __parse_array(json_str.match('[', ']'));
			else if (json_str[0] == '"')  // str类型
				return __parse_str(json_str);
			else if (isdigit(json_str[0]))  // number类型
				return __parse_num(json_str);
			else if (json_str == Str("null", 4))
				return Json(JsonType::JsonNull());
			else if (json_str == Str("true", 4))
				return Json(true);
			else if (json_str == Str("false", 5))
				return Json(false);
			else
				ValueError(json_str.__str__().str);
		}

		def parse(const Astring& json_str) -> Json
		{
			return parse(json_str.subme());
		}
	}
}