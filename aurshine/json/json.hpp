#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include <json/json_trait.hpp>
#include <law/printer.hpp>


namespace ayr
{
	namespace json
	{
		class Json : public Object
		{
		public:
			template<JsonTypeConcept T>
			Json(const T& item) : json_item(new std::remove_cvref_t<T>(item)), json_type(GetJsonTypeID<T>::ID) {}

			template<JsonTypeConcept T>
			Json(T&& item) : json_item(new std::remove_cvref_t<T>(std::forward<T>(item))), json_type(GetJsonTypeID<T>::ID) {}

			Json() : Json(Null()){}

			Json(Json&& json): Json() { this->swap(json); }

			Json(const Json& json): Json(){ *this = json; }

			~Json() { release(); }

			Json& operator=(const Json& other)
			{
				if (this != &other)
				{
					this->release();
					this->json_type = other.json_type;

					if (other.json_type == GetJsonTypeID<typename JsonType::JsonInt>::ID)
						this->json_item = new typename JsonType::JsonInt(other.transform<typename JsonType::JsonInt>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonFloat>::ID)
						this->json_item = new typename JsonType::JsonFloat(other.transform<typename JsonType::JsonFloat>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonBool>::ID)
						this->json_item = new typename JsonType::JsonBool(other.transform<typename JsonType::JsonBool>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonStr>::ID)
						this->json_item = new typename JsonType::JsonStr(other.transform<typename JsonType::JsonStr>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonArray>::ID)
						this->json_item = new typename JsonType::JsonArray(other.transform<typename JsonType::JsonArray>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonDict>::ID)
						this->json_item = new typename JsonType::JsonDict(other.transform<typename JsonType::JsonDict>());
					else if (other.json_type == GetJsonTypeID<typename JsonType::JsonNull>::ID)
						this->json_item = new typename JsonType::JsonNull(other.transform<typename JsonType::JsonNull>());
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
			int type() const { return this->json_type; }

			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			T& transform() 
			{ 
				check_type<T>();
				return *reinterpret_cast<T*>(this->json_item); 
			}

			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			const T& transform() const 
			{ 
				check_type<T>();
				return *reinterpret_cast<T*>(this->json_item); 
			}

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			T* transform_ptr() 
			{
				check_type<T>();
				return reinterpret_cast<T*>(this->json_item); 
			}

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			const T* transform_ptr() const 
			{
				check_type<T>();
				return reinterpret_cast<T*>(this->json_item);
			}

			template<JsonTypeConcept T>
			operator T() const { return transform<T>(); }

			// 转换为字符串
			const char* __str__() const
			{
				if (json_type == GetJsonTypeID<typename JsonType::JsonInt>::ID)
					memcpy__str_buffer__(transform<typename JsonType::JsonInt>());
				else if (json_type == GetJsonTypeID<typename JsonType::JsonFloat>::ID)
					memcpy__str_buffer__(transform<typename JsonType::JsonFloat>());
				else if (json_type == GetJsonTypeID<typename JsonType::JsonBool>::ID)
					memcpy__str_buffer__(transform<typename JsonType::JsonBool>() ? "true" : "false");
				else if (json_type == GetJsonTypeID<typename JsonType::JsonNull>::ID)
					memcpy__str_buffer__("null");
				else if (json_type == GetJsonTypeID<typename JsonType::JsonStr>::ID)
				{
					std::stringstream stream;
					stream << '"';
					stream << transform<typename JsonType::JsonStr>().__str__();
					stream << '"';
					memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
				}
				else if (json_type == GetJsonTypeID<typename JsonType::JsonArray>::ID)
				{
					std::stringstream stream;
					stream << "[";
					for (auto& item : transform<typename JsonType::JsonArray>())
						stream << item.__str__() << ", ";

					stream << "]\n";
					memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
				}
				else if (json_type == GetJsonTypeID<typename JsonType::JsonDict>::ID)
				{
					std::stringstream stream;
					stream << "{";
					for (auto& kv : transform<typename JsonType::JsonDict>())
						stream << "\"" << kv.first << "\": " << kv.second.__str__() << ", ";

					stream << "}\n";
					memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
				}

				return __str_buffer__;
			}

			// 尾部添加一个Json对象，需要当前Json对象为JsonArray类型
			Json& append(const Json& json)
			{
				check_type<typename JsonType::JsonArray>();

				transform<typename JsonType::JsonArray>().append(json);
				return *this;
			}

			Json& operator[] (const typename JsonType::JsonStr& key)
			{
				check_type<typename JsonType::JsonDict>();

				return transform<typename JsonType::JsonDict>()[key];
			}

			const Json& operator[] (const typename JsonType::JsonStr& key) const
			{
				check_type<typename JsonType::JsonDict>();

				return transform<typename JsonType::JsonDict>().at(key);
			}

			Json& operator[] (size_t index)
			{
				check_type<typename JsonType::JsonArray>();

				return transform<typename JsonType::JsonArray>()[index];
			}

			const Json& operator[] (size_t index) const
			{
				check_type<typename JsonType::JsonArray>();

				return transform<typename JsonType::JsonArray>()[index];
			}

			void pop(size_t index = -1)
			{
				check_type<typename JsonType::JsonArray>();
				transform<typename JsonType::JsonArray>().pop(index);
			}

			size_t size() const
			{
				if (json_type == GetJsonTypeID<typename JsonType::JsonArray>::ID)
					return transform<typename JsonType::JsonArray>().size();

				if (json_type == GetJsonTypeID<typename JsonType::JsonDict>::ID)
					return transform<typename JsonType::JsonDict>().size();

				error_assert(false, "Json type is not JSON_ARRAY or JSON_DICT");
			}

		private:
			// 释放内存
			void release()
			{
				if (json_type == GetJsonTypeID<typename JsonType::JsonInt>::ID)
					delete transform_ptr<typename JsonType::JsonInt>();
				else if (json_type == GetJsonTypeID<typename JsonType::JsonFloat>::ID)
					delete transform_ptr<typename JsonType::JsonFloat>();
				else if (json_type == GetJsonTypeID<typename JsonType::JsonBool>::ID)
					delete transform_ptr<typename JsonType::JsonBool>();
				else if (json_type == GetJsonTypeID<typename JsonType::JsonStr>::ID)
					delete transform_ptr<typename JsonType::JsonStr>();
				else if (json_type == GetJsonTypeID<typename JsonType::JsonArray>::ID)
					delete transform_ptr<typename JsonType::JsonArray>();
				else if (json_type == GetJsonTypeID<typename JsonType::JsonDict>::ID)
					delete transform_ptr<typename JsonType::JsonDict>();
			}

			// 检查类型是否正确
			template<JsonTypeStrictConcept T>
			void check_type() const
			{
				error_assert(json_type == GetJsonTypeID<T>::ID, std::format("Json type is not {}\n", typeid(T).name()));
			}

			void* json_item = nullptr;

			int8_t json_type = GetJsonTypeID<typename JsonType::JsonNull>::ID;
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
