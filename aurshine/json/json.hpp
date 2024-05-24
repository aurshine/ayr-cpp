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
	class Json;
	class Null {};

	using JsonInt    = long long;
	using JsonDouble = double;
	using JsonBool   = bool;
	using JsonStr    = std::string;
	using JsonArray  = std::vector<Json>;
	using JsonDict   = std::map<JsonStr, Json>;
	using JsonNull   = Null;

	template<typename T>
	using source_t = std::remove_reference_t<std::remove_const_t<T>>;

	template<typename T> struct GetJsonTypeEnum	  { constexpr static signed ID = -1;};
	template<> struct GetJsonTypeEnum<JsonInt>    { constexpr static signed ID = 0; };
	template<> struct GetJsonTypeEnum<JsonDouble> { constexpr static signed ID = 1; };
	template<> struct GetJsonTypeEnum<JsonBool>   { constexpr static signed ID = 2; };
	template<> struct GetJsonTypeEnum<JsonStr>    { constexpr static signed ID = 3; };
	template<> struct GetJsonTypeEnum<JsonArray>  { constexpr static signed ID = 4; };
	template<> struct GetJsonTypeEnum<JsonDict>   { constexpr static signed ID = 5; };
	template<> struct GetJsonTypeEnum<JsonNull>   { constexpr static signed ID = 6; };

	class Json : public Object
	{
	public:
		Json() : json_item(nullptr), json_type(GetJsonTypeEnum<JsonNull>::ID) {}

		template<typename T>
		Json(const T& item) : json_item(new source_t<T>(item)), json_type(GetJsonTypeEnum<source_t<T>>::ID) {}

		template<typename T>
		Json(T&& item): json_item(new source_t<T>(std::forward<T>(item))), json_type(GetJsonTypeEnum<source_t<T>>::ID) {}

		Json(Json&& json){ this->swap(json); }

		Json(const Json& json){ *this = json; }

		Json& operator=(const Json& json)
		{
			if (this == &json)
				return *this;

			delete this->json_item;

			if (json.json_type == GetJsonTypeEnum<JsonInt>::ID)
				this->json_item = new JsonInt(transform<JsonInt>());
			else if (json.json_type == GetJsonTypeEnum<JsonDouble>::ID)
				this->json_item = new JsonDouble(transform<JsonDouble>());
			else if (json.json_type == GetJsonTypeEnum<JsonBool>::ID)
				this->json_item = new JsonBool(transform<JsonBool>());
			else if (json.json_type == GetJsonTypeEnum<JsonStr>::ID)
				this->json_item = new JsonStr(transform<JsonStr>());
			else if (json.json_type == GetJsonTypeEnum<JsonArray>::ID)
				this->json_item = new JsonArray(transform<JsonArray>());
			else if (json.json_type == GetJsonTypeEnum<JsonDict>::ID)
				this->json_item = new JsonDict(transform<JsonDict>());

			this->json_type = json.json_type;

			return *this;
		}

		~Json()
		{ 
			if (json_type == GetJsonTypeEnum<JsonInt>::ID)
				delete reinterpret_cast<JsonInt*>(this->json_item);
			else if (json_type == GetJsonTypeEnum<JsonDouble>::ID)
				delete reinterpret_cast<JsonDouble*>(this->json_item);
			else if (json_type == GetJsonTypeEnum<JsonBool>::ID)
				delete reinterpret_cast<JsonBool*>(this->json_item);
			else if (json_type == GetJsonTypeEnum<JsonStr>::ID)
				delete reinterpret_cast<JsonStr*>(this->json_item);
			else if (json_type == GetJsonTypeEnum<JsonArray>::ID)
				delete reinterpret_cast<JsonArray*>(this->json_item);
			else if (json_type == GetJsonTypeEnum<JsonDict>::ID)
				delete reinterpret_cast<JsonDict*>(this->json_item);
		}

		void swap(Json& json)
		{
			std::swap(this->json_item, json.json_item);
			std::swap(this->json_type, json.json_type);
		}

		// 返回 Json 存储的对象类型
		signed type() { return this->json_type; }

		template<typename T>
		T& transform(){ return *reinterpret_cast<T*>(this->json_item); }

		template<typename T>
		const T& transform() const { return *reinterpret_cast<T*>(this->json_item); }

		template<typename T>
		operator T() const { return *reinterpret_cast<T*>(this->json_item); }

		std::string to_string() const override
		{
			std::string buffer;
			if (json_type == GetJsonTypeEnum<JsonInt>::ID)
				buffer = std::to_string(transform<JsonInt>());
			else if (json_type == GetJsonTypeEnum<JsonDouble>::ID)
				buffer = std::to_string(transform<JsonDouble>());
			else if (json_type == GetJsonTypeEnum<JsonBool>::ID)
				buffer = transform<JsonBool>() ? "true" : "false";
			else if (json_type == GetJsonTypeEnum<JsonNull>::ID)
				buffer = "null";
			else if (json_type == GetJsonTypeEnum<JsonStr>::ID)
				buffer = "\"" + transform<JsonStr>() + "\"";
			else if (json_type == GetJsonTypeEnum<JsonArray>::ID)
			{
				buffer = "[";
				for (auto& item : transform<JsonArray>())
					buffer += item.to_string() + ", ";
				buffer.pop_back(), buffer.pop_back();
				buffer += "]\n";
			}
			else if (json_type == GetJsonTypeEnum<JsonDict>::ID)
			{
				buffer = "{";
				for (auto& kv : transform<JsonDict>())
					buffer += "\"" + kv.first + "\": " + kv.second.to_string() + ", ";
				
				buffer.pop_back(), buffer.pop_back();
				buffer += "}\n";
			}

			return buffer;
		}

		Json& append(const Json& json)
		{
			error_assert(json_type == GetJsonTypeEnum<JsonArray>::ID, "Json type is not JSON_ARRAY");

			transform<JsonArray>().push_back(json);
			return *this;
		}

		Json& operator[] (const JsonStr& key)
		{
			error_assert(json_type == GetJsonTypeEnum<JsonDict>::ID, "Json type is not JSON_DICT");

			return transform<JsonDict>()[key];
		}

		const Json& operator[] (const JsonStr& key) const
		{
			error_assert(json_type == GetJsonTypeEnum<JsonDict>::ID, "Json type is not JSON_DICT");

			return transform<JsonDict>().at(key);
		}

		Json& operator[] (size_t index)
		{
			error_assert(json_type == GetJsonTypeEnum<JsonArray>::ID, "Json type is not JSON_ARRAY");

			return transform<JsonArray>()[index];
		}

		const Json& operator[] (size_t index) const
		{
			error_assert(json_type == GetJsonTypeEnum<JsonArray>::ID, "Json type is not JSON_ARRAY");

			return transform<JsonArray>().at(index);
		}

		void pop(size_t index = -1)
		{
			error_assert(json_type == GetJsonTypeEnum<JsonArray>::ID, "Json type is not JSON_ARRAY");


			if (index == -1)
				index = transform<JsonArray>().size() - 1;
			transform<JsonArray>().erase(transform<JsonArray>().begin() + index);
		}

		size_t size() const
		{
			if (json_type == GetJsonTypeEnum<JsonArray>::ID)
				return transform<JsonArray>().size();
			
			if (json_type == GetJsonTypeEnum<JsonDict>::ID)
				return transform<JsonDict>().size();

			error_assert(false, "Json type is not JSON_ARRAY or JSON_DICT");
		}
	private:
		void* json_item = nullptr;

		signed json_type = GetJsonTypeEnum<JsonNull>::ID;
	};
}
