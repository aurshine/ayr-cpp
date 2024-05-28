#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cassert>
#include <functional>
#include "json_trait.hpp"
#include "../law/law.hpp"


namespace ayr
{
	namespace json
	{
		class Json : public Object
		{
		public:
			Json() : json_item(nullptr), json_type(GetJsonTypeEnum<typename JsonType::JsonNull>::ID) {}

			template<JsonTypeConcept T>
			Json(const T& item) : json_item(new std::remove_cvref_t<T>(item)), json_type(GetJsonTypeEnum<T>::ID) {}

			template<JsonTypeConcept T>
			Json(T&& item) : json_item(new std::remove_cvref_t<T>(std::forward<T>(item))), json_type(GetJsonTypeEnum<T>::ID) {}

			Json(Json&& json) noexcept { this->swap(json); }

			Json(const Json& json) { *this = json; }

			~Json() { release(); }

			Json& operator=(const Json& json)
			{
				if (this != &json)
				{
					this->release();
					this->json_type = json.json_type;

					if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonInt>::ID)
						this->json_item = new typename JsonType::JsonInt(transform<typename JsonType::JsonInt>());
					else if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonFloat>::ID)
						this->json_item = new typename JsonType::JsonFloat(transform<typename JsonType::JsonFloat>());
					else if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonBool>::ID)
						this->json_item = new typename JsonType::JsonBool(transform<typename JsonType::JsonBool>());
					else if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonStr>::ID)
						this->json_item = new typename JsonType::JsonStr(transform<typename JsonType::JsonStr>());
					else if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID)
						this->json_item = new typename JsonType::JsonArray(transform<typename JsonType::JsonArray>());
					else if (json.json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID)
						this->json_item = new typename JsonType::JsonDict(transform<typename JsonType::JsonDict>());
					else
						error_assert(false, std::format("ValueError: json_type can not be {}", type()));
				}

				return *this;
			}

			void swap(Json& json) noexcept
			{
				std::swap(this->json_item, json.json_item);
				std::swap(this->json_type, json.json_type);
			}

			// 返回 Json 存储的对象类型
			int type() { return this->json_type; }

			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			T& transform() { return *reinterpret_cast<T*>(this->json_item); }

			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			const T& transform() const { return *reinterpret_cast<T*>(this->json_item); }

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			T* transform_ptr() { return reinterpret_cast<T*>(this->json_item); }

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			const T* transform_ptr() const { return reinterpret_cast<T*>(this->json_item); }

			template<JsonTypeConcept T>
			operator T() const { return *reinterpret_cast<T*>(this->json_item); }

			// 转换为字符串
			const char* __str__() const override
			{
				std::string buffer;
				if (json_type == GetJsonTypeEnum<typename JsonType::JsonInt>::ID)
					buffer = std::to_string(transform<typename JsonType::JsonInt>());
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonFloat>::ID)
					buffer = std::to_string(transform<typename JsonType::JsonFloat>());
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonBool>::ID)
					buffer = transform<typename JsonType::JsonBool>() ? "true" : "false";
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonNull>::ID)
					buffer = "null";
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonStr>::ID)
					buffer = std::format(R"("{}")", transform<typename JsonType::JsonStr>());
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID)
				{
					buffer = "[";
					for (auto& item : transform<typename JsonType::JsonArray>())
						buffer = buffer + item.__str__() + ", ";

					buffer.pop_back(), buffer.pop_back();
					buffer += "]\n";
				}
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID)
				{
					buffer = "{";
					for (auto& kv : transform<typename JsonType::JsonDict>())
						buffer = buffer + "\"" + kv.first + "\": " + kv.second.__str__() + ", ";

					buffer.pop_back(), buffer.pop_back();
					buffer += "}\n";
				}

				return buffer.c_str();
			}

			// 尾部添加一个Json对象，需要当前Json对象为JsonArray类型
			Json& append(const Json& json)
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID, "Json type is not JSON_ARRAY");

				transform<typename JsonType::JsonArray>().push_back(json);
				return *this;
			}

			Json& operator[] (const typename JsonType::JsonStr& key)
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID, "Json type is not JSON_DICT");

				return transform<typename JsonType::JsonDict>()[key];
			}

			const Json& operator[] (const typename JsonType::JsonStr& key) const
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID, "Json type is not JSON_DICT");

				return transform<typename JsonType::JsonDict>().at(key);
			}

			Json& operator[] (size_t index)
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID, "Json type is not JSON_ARRAY");

				return transform<typename JsonType::JsonArray>()[index];
			}

			const Json& operator[] (size_t index) const
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID, "Json type is not JSON_ARRAY");

				return transform<typename JsonType::JsonArray>().at(index);
			}

			void pop(size_t index = -1)
			{
				error_assert(json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID, "Json type is not JSON_ARRAY");


				if (index == -1)
					index = transform<typename JsonType::JsonArray>().size() - 1;
				transform<typename JsonType::JsonArray>().erase(transform<typename JsonType::JsonArray>().begin() + index);
			}

			size_t size() const
			{
				if (json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID)
					return transform<typename JsonType::JsonArray>().size();

				if (json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID)
					return transform<typename JsonType::JsonDict>().size();

				error_assert(false, "Json type is not JSON_ARRAY or JSON_DICT");
			}

		private:
			void release()
			{
				if (json_type == GetJsonTypeEnum<typename JsonType::JsonInt>::ID)
					delete reinterpret_cast<typename JsonType::JsonInt*>(this->json_item);
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonFloat>::ID)
					delete reinterpret_cast<typename JsonType::JsonFloat*>(this->json_item);
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonBool>::ID)
					delete reinterpret_cast<typename JsonType::JsonBool*>(this->json_item);
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonStr>::ID)
					delete reinterpret_cast<typename JsonType::JsonStr*>(this->json_item);
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonArray>::ID)
					delete reinterpret_cast<typename JsonType::JsonArray*>(this->json_item);
				else if (json_type == GetJsonTypeEnum<typename JsonType::JsonDict>::ID)
					delete reinterpret_cast<typename JsonType::JsonDict*>(this->json_item);
			}

			void* json_item = nullptr;

			int8_t json_type = GetJsonTypeEnum<typename JsonType::JsonNull>::ID;
		};

		template<typename T>
		JsonType::JsonInt make_int(T&& value)
		{
			using RT = std::remove_cvref_t<T>;

			if constexpr (std::is_arithmetic_v<RT>)
				return static_cast<JsonType::JsonInt>(value);

			error_assert(false, "Unsupported " + std::string(typeid(T).name()) + " for make_int");
			return 0;
		}

		template<typename T>
		JsonType::JsonInt make_float(T&& value)
		{
			using RT = std::remove_cvref_t<T>;

			if constexpr (std::is_arithmetic_v<RT>)
				return static_cast<JsonType::JsonFloat>(value);

			error_assert(false, "Unsupported " + std::string(typeid(T).name()) + " for make_float");
			return 0;
		}

		template<typename T>
		JsonType::JsonBool make_bool(T&& value)
		{
			return static_cast<JsonType::JsonBool>(value);
		}
	}
}
