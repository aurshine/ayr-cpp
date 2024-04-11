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
		using JsonStr = std::string;

		using JsonArray = std::vector<Json>;

		using JsonDict = std::map<JsonStr, Json>;

		Json()
		{
			reset();
			this->json_type = JSON_NULL;
		}

		Json(long long item)
		{
			this->int_item = new long long(item);
			this->json_type = JSON_INT;
		}

		Json(double item)
		{
			this->double_item = new double(item);
			json_type = JSON_DOUBLE;
		}

		Json(bool item)
		{
			this->bool_item = new bool(item);
			this->json_type = JSON_BOOL;
		}

		Json(const char* item)
		{
			this->str_item = new JsonStr(item);
			this->json_type = JSON_STR;
		}

		Json(JsonStr&& item)
		{
			this->str_item = new JsonStr(std::move(item));
			this->json_type = JSON_STR;
		}

		Json(JsonArray&& item)
		{
			this->arr_item = new JsonArray(std::move(item));
			this->json_type = JSON_ARRAY;
		}

		Json(JsonDict&& item)
		{
			this->dict_item = new JsonDict(std::move(item));
			this->json_type = JSON_DICT;
		}

		Json(Json&& json) noexcept
		{
			release();
			if (json.int_item)
				this->int_item = json.int_item;
			else if (json.double_item)
				this->double_item = json.double_item;
			else if (json.bool_item)
				this->bool_item = json.bool_item;
			else if (json.str_item)
				this->str_item = json.str_item;
			else if (json.arr_item)
				this->arr_item = json.arr_item;
			else if (json.dict_item)
				this->dict_item = json.dict_item;

			this->json_type = json.json_type;
			json.reset();
		}

		Json& operator= (Json&& json) noexcept
		{
			if (this != &json)
			{
				release();
				if (json.int_item)
					this->int_item = json.int_item;
				else if (json.double_item)
					this->double_item = json.double_item;
				else if (json.bool_item)
					this->bool_item = json.bool_item;
				else if (json.str_item)
					this->str_item = json.str_item;
				else if (json.arr_item)
					this->arr_item = json.arr_item;
				else if (json.dict_item)
					this->dict_item = json.dict_item;

				this->json_type = json.json_type;
				json.reset();
			}

			return *this;
		}

		Json(const Json& json) = delete;


		~Json()
		{
			release();
		}

		void release()
		{
			switch (this->json_type)
			{
			case JSON_INT:
				delete this->int_item;
				break;
			case JSON_DOUBLE:
				delete this->double_item;
				break;
			case JSON_BOOL:
				delete this->bool_item;
				break;
			case JSON_STR:
				delete this->str_item;
				break;
			case JSON_ARRAY:
				delete this->arr_item;
				break;
			case JSON_DICT:
				delete this->dict_item;
				break;
			}

			this->json_type = JSON_NULL;
			reset();
		}

		void reset()
		{
			this->int_item = nullptr;
			this->double_item = nullptr;
			this->bool_item = nullptr;
			this->str_item = nullptr;
			this->arr_item = nullptr;
			this->dict_item = nullptr;
		}


		// 返回 Json 存储的对象类型
		JsonType type()
		{
			return this->json_type;
		}

		long long& as_int()
		{
			error_assert(json_type == JSON_INT, "Json type is not JSON_INT");
			return *this->int_item;
		}

		long long as_int() const
		{
			error_assert(json_type == JSON_INT, "Json type is not JSON_INT");
			return *this->int_item;
		}

		double& as_double()
		{
			error_assert(json_type == JSON_DOUBLE, "Json type is not JSON_DOUBLE");
			return *this->double_item;
		}

		double as_double() const
		{
			error_assert(json_type == JSON_DOUBLE, "Json type is not JSON_DOUBLE");
			return *this->double_item;
		}

		bool& as_bool()
		{
			error_assert(json_type == JSON_BOOL, "Json type is not JSON_BOOL");
			return *this->bool_item;
		}

		bool as_bool() const
		{
			error_assert(json_type == JSON_BOOL, "Json type is not JSON_BOOL");
			return *this->bool_item;
		}

		JsonStr& as_str()
		{
			error_assert(json_type == JSON_STR, "Json type is not JSON_STR");
			return *this->str_item;
		}

		JsonStr as_str() const
		{
			error_assert(json_type == JSON_STR, "Json type is not JSON_STR");
			return *this->str_item;
		}

		JsonArray& as_array()
		{
			error_assert(json_type == JSON_ARRAY, "Json type is not JSON_ARRAY");
			return *this->arr_item;
		}

		JsonDict& as_dict()
		{
			error_assert(json_type == JSON_DICT, "Json type is not JSON_DICT");
			return *this->dict_item;
		}

		std::string to_string() const override
		{
			std::string str;
			if (json_type == JSON_INT)
				str = std::to_string(*this->int_item);
			else if (json_type == JSON_DOUBLE)
				str = std::to_string(*this->double_item);
			else if (json_type == JSON_BOOL)
				str = *this->bool_item ? "true" : "false";
			else if (json_type == JSON_NULL)
				str = "null";
			else if (json_type == JSON_STR)
				str = "\"" + *this->str_item + "\"";
			else if (json_type == JSON_ARRAY)
			{
				str = "[";
				for (auto& item : *this->arr_item)
					str += item.to_string() + ", ";
				str.pop_back(), str.pop_back();
				str += "]\n";
			}
			else if (json_type == JSON_DICT)
			{
				str = "{";
				for (auto& kv : *this->dict_item)
				{
					str += "\"" + kv.first + "\": " + kv.second.to_string() + ", ";
				}
				str.pop_back(), str.pop_back();
				str += "}\n";
			}

			return str;
		}
	private:
		union
		{
			long long* int_item;

			double* double_item;

			bool* bool_item;

			JsonStr* str_item;

			JsonArray* arr_item;

			JsonDict* dict_item;
		};

		JsonType json_type;
	};
}
