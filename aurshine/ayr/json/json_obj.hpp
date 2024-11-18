#ifndef AYR_JSON_JSON_OBJ_HPP
#define AYR_JSON_JSON_OBJ_HPP

#include "json_trait.hpp"


namespace ayr
{
	namespace json
	{
		class Json : public Object<Json>
		{
		public:
			template<JsonTypeStrictConcept T>
			Json(T&& item) : json_item(new T(std::forward<T>(item))), json_type(GetJsonTypeIDStrict<T>::ID) {}

			Json() : Json(Null()) {}

			Json(Json&& other) noexcept
			{
				json_type = other.json_type;
				json_item = other.json_item;

				other.json_item = nullptr;
				other.json_type = GetJsonTypeIDStrict<JsonType::JsonNull>::ID;
			}

			Json(const Json& other)
			{
				json_type = other.json_type;

				switch (json_type)
				{
				case GetJsonTypeID<JsonType::JsonInt>::ID:
					this->json_item = new JsonType::JsonInt(other.transform<JsonType::JsonInt>());
					break;
				case GetJsonTypeID<JsonType::JsonFloat>::ID:
					this->json_item = new JsonType::JsonFloat(other.transform<JsonType::JsonFloat>());
					break;
				case GetJsonTypeID<JsonType::JsonBool>::ID:
					this->json_item = new JsonType::JsonBool(other.transform<JsonType::JsonBool>());
					break;
				case GetJsonTypeID<JsonType::JsonStr>::ID:
					this->json_item = new JsonType::JsonStr(other.transform<JsonType::JsonStr>());
					break;
				case GetJsonTypeID<JsonType::JsonArray>::ID:
					this->json_item = new JsonType::JsonArray(other.transform<JsonType::JsonArray>());
					break;
				case GetJsonTypeID<JsonType::JsonDict>::ID:
					this->json_item = new JsonType::JsonDict(other.transform<JsonType::JsonDict>());
					break;
				case GetJsonTypeID<JsonType::JsonNull>::ID:
					this->json_item = new JsonType::JsonNull(other.transform<JsonType::JsonNull>());
					break;
				default:
					ValueError(std::format("json_type {} is not valid", json_type));
				}
			}

			~Json() { release(); }

			Json& operator=(const Json& other)
			{
				if (this != &other)
				{
					release();
					json_type = other.json_type;

					switch (json_type)
					{
					case GetJsonTypeID<JsonType::JsonInt>::ID:
						this->json_item = new JsonType::JsonInt(other.transform<JsonType::JsonInt>());
						break;
					case GetJsonTypeID<JsonType::JsonFloat>::ID:
						this->json_item = new JsonType::JsonFloat(other.transform<JsonType::JsonFloat>());
						break;
					case GetJsonTypeID<JsonType::JsonBool>::ID:
						this->json_item = new JsonType::JsonBool(other.transform<JsonType::JsonBool>());
						break;
					case GetJsonTypeID<JsonType::JsonStr>::ID:
						this->json_item = new JsonType::JsonStr(other.transform<JsonType::JsonStr>());
						break;
					case GetJsonTypeID<JsonType::JsonArray>::ID:
						this->json_item = new JsonType::JsonArray(other.transform<JsonType::JsonArray>());
						break;
					case GetJsonTypeID<JsonType::JsonDict>::ID:
						this->json_item = new JsonType::JsonDict(other.transform<JsonType::JsonDict>());
						break;
					case GetJsonTypeID<JsonType::JsonNull>::ID:
						this->json_item = new JsonType::JsonNull(other.transform<JsonType::JsonNull>());
						break;
					default:
						ValueError(std::format("json_type {} is not valid", json_type));
					}
				}

				return *this;
			}


			Json& operator=(Json&& other) noexcept
			{
				if (this != &other)
				{
					release();
					json_type = other.json_type;
					json_item = other.json_item;

					other.json_item = nullptr;
					other.json_type = GetJsonTypeID<JsonType::JsonNull>::ID;
				}

				return *this;
			}


			void swap(Json& json) noexcept
			{
				std::swap(this->json_item, json.json_item);
				std::swap(this->json_type, json.json_type);
			}

			template<JsonTypeStrictConcept T>
			bool is() const { return json_type == GetJsonTypeID<T>::ID; }

			// 返回 Json 存储的对象类型ID
			static CString type_name(int8_t id)
			{
				switch (id)
				{
				case GetJsonTypeID<JsonType::JsonInt>::ID:
					return "JsonInt";
				case GetJsonTypeID<JsonType::JsonFloat>::ID:
					return "JsonFloat";
				case GetJsonTypeID<JsonType::JsonBool>::ID:
					return "JsonBool";
				case GetJsonTypeID<JsonType::JsonStr>::ID:
					return "JsonStr";
				case GetJsonTypeID<JsonType::JsonArray>::ID:
					return "JsonArray";
				case GetJsonTypeID<JsonType::JsonDict>::ID:
					return "JsonDict";
				case GetJsonTypeID<JsonType::JsonNull>::ID:
					return "JsonNull";
				default:
					ValueError(std::format("json_type {} is not valid", id));
				}
			}

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			T* transform_ptr()
			{
				if (!is<T>())
					ValueError(std::format("Json type is not {} but {}",
						type_name(GetJsonTypeID<T>::ID),
						type_name(json_type))
					);

				return reinterpret_cast<T*>(this->json_item);
			}

			// 转换为指定类型，返回转换后对象的指针
			template<JsonTypeStrictConcept T>
			const T* transform_ptr() const
			{
				if (!is<T>())
					ValueError(std::format("Json type is not {} but {}",
						type_name(GetJsonTypeID<T>::ID),
						type_name(json_type))
					);
				return reinterpret_cast<T*>(this->json_item);
			}


			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			T& transform() { return *transform_ptr<T>(); }

			// 转换为指定类型，返回转换后对象的引用
			template<JsonTypeStrictConcept T>
			const T& transform() const { return *transform_ptr<T>(); }


			template<JsonTypeStrictConcept T>
			operator T() const { return transform<T>(); }

			// 转换为字符串
			CString __str__() const
			{
				switch (json_type)
				{
				case GetJsonTypeID<JsonType::JsonInt>::ID:
					return cstr(transform<JsonType::JsonInt>());
				case GetJsonTypeID<JsonType::JsonFloat>::ID:
					return cstr(transform<JsonType::JsonFloat>());
				case GetJsonTypeID<JsonType::JsonBool>::ID:
					return cstr(transform<JsonType::JsonBool>());
				case GetJsonTypeID<JsonType::JsonStr>::ID:
					return cstr("\""as + transform<JsonType::JsonStr>() + "\""as);
				case GetJsonTypeID<JsonType::JsonArray>::ID:
					return cstr(transform<JsonType::JsonArray>());
				case GetJsonTypeID<JsonType::JsonDict>::ID:
					return cstr(transform<JsonType::JsonDict>());
				case GetJsonTypeID<JsonType::JsonNull>::ID:
					return cstr("null");
				default:
					ValueError(std::format("json_type {} is not valid", json_type));
				}
			}

			// 尾部添加一个Json对象，需要当前Json对象为JsonArray类型
			Json& append(const Json& json)
			{
				transform<JsonType::JsonArray>().append(json);
				return *this;
			}

			Json& operator[] (const JsonType::JsonStr& key)
			{
				return transform<JsonType::JsonDict>()[key];
			}

			const Json& operator[] (const JsonType::JsonStr& key) const
			{
				return transform<JsonType::JsonDict>().get(key);
			}

			Json& operator[] (size_t index)
			{
				return transform<JsonType::JsonArray>()[index];
			}

			const Json& operator[] (size_t index) const
			{
				return transform<JsonType::JsonArray>()[index];
			}

			void pop(size_t index = -1)
			{
				transform<JsonType::JsonArray>().pop(index);
			}

			size_t size() const
			{
				if (json_type == GetJsonTypeID<JsonType::JsonArray>::ID)
					return transform<JsonType::JsonArray>().size();

				if (json_type == GetJsonTypeID<JsonType::JsonDict>::ID)
					return transform<JsonType::JsonDict>().size();

				RuntimeError("Json type is not JSON_ARRAY or JSON_DICT");
			}

		private:
			// 释放内存
			void release()
			{
				switch (json_type)
				{
				case GetJsonTypeID<JsonType::JsonInt>::ID:
					delete transform_ptr<JsonType::JsonInt>();
					break;
				case GetJsonTypeID<JsonType::JsonFloat>::ID:
					delete transform_ptr<JsonType::JsonFloat>();
					break;
				case GetJsonTypeID<JsonType::JsonBool>::ID:
					delete transform_ptr<JsonType::JsonBool>();
					break;
				case GetJsonTypeID<JsonType::JsonStr>::ID:
					delete transform_ptr<JsonType::JsonStr>();
					break;
				case GetJsonTypeID<JsonType::JsonArray>::ID:
					delete transform_ptr<JsonType::JsonArray>();
					break;
				case GetJsonTypeID<JsonType::JsonDict>::ID:
					delete transform_ptr<JsonType::JsonDict>();
					break;
				case GetJsonTypeID<JsonType::JsonNull>::ID:
					delete transform_ptr<JsonType::JsonNull>();
					break;
				default:
					ValueError(std::format("json_type {} is not valid", json_type));
				}
			}

			void* json_item;

			int8_t json_type;
		};

		template<typename T>
		constexpr inline JsonType::JsonInt make_int(T&& value)
		{
			using RT = std::remove_cvref_t<T>;

			if constexpr (std::is_arithmetic_v<RT>)
				return static_cast<JsonType::JsonInt>(value);

			RuntimeError(std::format("Unsupported {} for make int", dtype(T)));
			return None<JsonType::JsonInt>;
		}

		template<typename T>
		constexpr inline JsonType::JsonFloat make_float(T&& value)
		{
			using RT = std::remove_cvref_t<T>;

			if constexpr (std::is_arithmetic_v<RT>)
				return static_cast<JsonType::JsonFloat>(value);

			RuntimeError(std::format("Unsupported {} for make float", dtype(T)));
			return None<JsonType::JsonFloat>;
		}

		template<typename T>
		constexpr inline JsonType::JsonBool make_bool(T&& value)
		{
			return static_cast<JsonType::JsonBool>(value);
		}
	}
}
#endif