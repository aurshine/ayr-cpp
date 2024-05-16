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
			delete this->json_item;
			this->json_item = json.json_item;
			json.json_item = nullptr;
			this->json_type = json.json_type;
		}

		Json(const Json& json)
		{
			*this = json;
		}

		Json& operator=(const Json& json)
		{
			if (this == &json)
				return *this;

			delete this->json_item;
			if (json.json_type == JSON_INT)
				this->json_item = new JsonInt(json.as_int());
			else if (json.json_type == JSON_DOUBLE)
				this->json_item = new JsonDouble(json.as_double());
			else if (json.json_type == JSON_BOOL)
				this->json_item = new JsonBool(json.as_bool());
			else if (json.json_type == JSON_STR)
				this->json_item = new JsonStr(json.as_str());
			else if (json.json_type == JSON_ARRAY)
				this->json_item = new JsonArray(json.as_array());
			else if (json.json_type == JSON_DICT)
				this->json_item = new JsonDict(json.as_dict());
			this->json_type = json.json_type;
			return *this;
		}

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

		operator JsonInt() const
		{
			error_assert(json_type == JSON_INT, "Json type is not JSON_INT");
			return *reinterpret_cast<JsonInt*>(this->json_item);
		}

		operator JsonDouble() const
		{
			error_assert(json_type == JSON_DOUBLE, "Json type is not JSON_DOUBLE");
			return *reinterpret_cast<JsonDouble*>(this->json_item);
		}

		operator JsonBool() const
		{
			error_assert(json_type == JSON_BOOL, "Json type is not JSON_BOOL");
			return *reinterpret_cast<JsonBool*>(this->json_item);
		}

		operator JsonStr() const
		{
			error_assert(json_type == JSON_STR, "Json type is not JSON_STR");
			return *reinterpret_cast<JsonStr*>(this->json_item);
		}

		operator JsonArray() const
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return *reinterpret_cast<JsonArray*>(this->json_item);
		}

		operator JsonDict() const
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

		Json& append(Json&& json)
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			this->as_array().emplace_back(std::move(json));
			return *this;
		}

		Json& operator[] (const JsonStr& key)
		{
			error_assert(json_type == JSON_DICT, "Json type is not JSON_DICT");
			return this->as_dict()[key];
		}

		const Json& operator[] (const JsonStr& key) const
		{
			error_assert(json_type == JSON_DICT, "Json type is not JSON_DICT");
			return this->as_dict().at(key);
		}

		Json& operator[] (size_t index)
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return this->as_array()[index];
		}

		const Json& operator[] (size_t index) const
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return this->as_array().at(index);
		}

		void pop(size_t index = -1)
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");

			if (index == -1)
				index = this->as_array().size() - 1;
			this->as_array().erase(this->as_array().begin() + index);
		}

		size_t size() const
		{
			if (json_type == JSON_ARRAY)
				return this->as_array().size();
			else if (json_type == JSON_DICT)
				return this->as_dict().size();
			error_assert(false, "Json type is not JSON_ARRAY or JSON_DICT");
		}
	private:
		void* json_item;

		JsonType json_type;
	};
}
