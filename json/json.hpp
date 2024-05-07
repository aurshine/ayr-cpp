#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cassert>
#include "../law/law.hpp"


namespace ayr
{
	using JsonType = int;

	constexpr JsonType JSON_INT = 0, JSON_DOUBLE = 1, JSON_BOOL = 2;

	constexpr JsonType JSON_STR = 3, JSON_ARRAY = 4, JSON_DICT = 5, JSON_NULL = 6;

	class Json : public Object
	{
	public:
		using JsonInt = long long;

		using JsonDouble = double;

		using JsonBool = bool;

		using JsonStr = std::string;

		using JsonArray = std::vector<Json>;

		using JsonDict = std::map<JsonStr, Json>;

		Json()
			: json_item(nullptr), json_type(JSON_NULL)
		{
		}

		Json(long long item)
		{
			this->json_item = new long long(item);
			this->json_type = JSON_INT;
		}

		Json(double item)
		{
			this->json_item = new double(item);
			json_type = JSON_DOUBLE;
		}

		Json(bool item)
		{
			this->json_item = new bool(item);
			this->json_type = JSON_BOOL;
		}

		Json(const char* item)
		{
			this->json_item = new JsonStr(item);
			this->json_type = JSON_STR;
		}

		Json(JsonStr&& item)
		{
			this->json_item = new JsonStr(std::move(item));
			this->json_type = JSON_STR;
		}

		Json(JsonArray&& item)
		{
			this->json_item = new JsonArray(std::move(item));
			this->json_type = JSON_ARRAY;
		}

		Json(JsonDict&& item)
		{
			this->json_item = new JsonDict(std::move(item));
			this->json_type = JSON_DICT;
		}

		Json(Json&& json) noexcept
		{
			this->json_item = json.json_item;
			json.json_item = nullptr;
			this->json_type = json.json_type;
		}

		Json& operator= (Json&& json) noexcept
		{
			if (this != &json)
			{
				this->json_item = json.json_item;
				json.json_item = nullptr;
				this->json_type = json.json_type;
			}

			return *this;
		}

		Json(const Json& json) = delete;


		~Json()
		{
			delete this->json_item;
		}

		// 返回 Json 存储的对象类型
		JsonType type()
		{
			return this->json_type;
		}

		JsonInt& as_int()
		{
			error_assert(json_type == JSON_INT, "Json type is not JSON_INT");

			return *reinterpret_cast<JsonInt*>(this->json_item);
		}

		JsonInt as_int() const
		{
			error_assert(json_type == JSON_INT, "Json type is not JSON_INT");
			return *reinterpret_cast<JsonInt*>(this->json_item);
		}

		JsonDouble& as_double()
		{
			error_assert(json_type == JSON_DOUBLE, "Json type is not JSON_DOUBLE");
			return *reinterpret_cast<JsonDouble*>(this->json_item);
		}

		JsonDouble as_double() const
		{
			error_assert(json_type == JSON_DOUBLE, "Json type is not JSON_DOUBLE");
			return *reinterpret_cast<JsonDouble*>(this->json_item);
		}

		JsonBool& as_bool()
		{
			error_assert(json_type == JSON_BOOL, "Json type is not JSON_BOOL");
			return *reinterpret_cast<JsonBool*>(this->json_item);
		}

		JsonBool as_bool() const
		{
			error_assert(json_type == JSON_BOOL, "Json type is not JSON_BOOL");
			return *reinterpret_cast<JsonBool*>(this->json_item);
		}

		JsonStr& as_str()
		{
			error_assert(json_type == JSON_STR, "Json type is not JSON_STR");
			return *reinterpret_cast<JsonStr*>(this->json_item);
		}

		const JsonStr& as_str() const
		{
			error_assert(json_type == JSON_STR, "Json type is not JSON_STR");
			return *reinterpret_cast<JsonStr*>(this->json_item);
		}

		JsonArray& as_array()
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return *reinterpret_cast<JsonArray*>(this->json_item);
		}

		const JsonArray& as_array() const
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return *reinterpret_cast<JsonArray*>(this->json_item);
		}

		JsonDict& as_dict()
		{
			error_assert(json_type == JSON_DICT, "Json type is not JSON_DICT");
			return *reinterpret_cast<JsonDict*>(this->json_item);
		}

		const JsonDict& as_dict() const
		{
			error_assert(json_type == JSON_DICT, "Json type is not JSON_DICT");
			return *reinterpret_cast<JsonDict*>(this->json_item);
		}

		std::string to_string() const override
		{
			std::string str;
			if (json_type == JSON_INT)
				str = std::to_string(this->as_int());
			else if (json_type == JSON_DOUBLE)
				str = std::to_string(this->as_double());
			else if (json_type == JSON_BOOL)
				str = this->as_bool() ? "true" : "false";
			else if (json_type == JSON_NULL)
				str = "null";
			else if (json_type == JSON_STR)
				str = "\"" + this->as_str() + "\"";
			else if (json_type == JSON_ARRAY)
			{
				str = "[";
				for (auto& item : this->as_array())
					str += item.to_string() + ", ";
				str.pop_back(), str.pop_back();
				str += "]\n";
			}
			else if (json_type == JSON_DICT)
			{
				str = "{";
				for (auto& kv : this->as_dict())
				{
					str += "\"" + kv.first + "\": " + kv.second.to_string() + ", ";
				}
				str.pop_back(), str.pop_back();
				str += "}\n";
			}

			return str;
		}
	private:
		void* json_item;

		JsonType json_type;
	};
}
