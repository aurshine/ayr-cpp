#pragma once
#include <json/json_trait.hpp>
#include <law/detail/printer.hpp>


namespace ayr
{
	namespace json
	{
		class Json : public Object
		{
		public:
			template<JsonTypeStrictConcept T>
			Json(const T& item) : json_item(new T(item)), json_type(GetJsonTypeIDStrict<T>::ID) {}

			template<JsonTypeStrictConcept T>
			Json(T&& item) : json_item(new T(std::move(item))), json_type(GetJsonTypeIDStrict<T>::ID) {}

			Json() : Json(Null()){}

			Json(Json&& json): Json() { this->swap(json); }

			Json(const Json& json): Json(){ *this = json; }

			~Json() { release(); }

			Json& operator=(const Json& other)
			{
				if (this != &other)
				{
					release();
					json_type = other.json_type;

					switch (json_type)
					{
						case GetJsonTypeID<typename JsonType::JsonInt>::ID:
							this->json_item = new typename JsonType::JsonInt(other.transform<typename JsonType::JsonInt>());
							break;
						case GetJsonTypeID<typename JsonType::JsonFloat>::ID:
							this->json_item = new typename JsonType::JsonFloat(other.transform<typename JsonType::JsonFloat>());
							break;
						case GetJsonTypeID<typename JsonType::JsonBool>::ID:
							this->json_item = new typename JsonType::JsonBool(other.transform<typename JsonType::JsonBool>());
							break;
						case GetJsonTypeID<typename JsonType::JsonStr>::ID:
							this->json_item = new typename JsonType::JsonStr(other.transform<typename JsonType::JsonStr>());
							break;
						case GetJsonTypeID<typename JsonType::JsonArray>::ID:
							this->json_item = new typename JsonType::JsonArray(other.transform<typename JsonType::JsonArray>());
							break;
						case GetJsonTypeID<typename JsonType::JsonDict>::ID:
							this->json_item = new typename JsonType::JsonDict(other.transform<typename JsonType::JsonDict>());
							break;
						case GetJsonTypeID<typename JsonType::JsonNull>::ID:
							this->json_item = new typename JsonType::JsonNull(other.transform<typename JsonType::JsonNull>());
							break;
						default:
							ValueError(std::format("json_type can not be {}"), type());
					}
				}
				return *this;
			}

			void swap(Json& json) noexcept
			{
				std::swap(this->json_item, json.json_item);
				std::swap(this->json_type, json.json_type);
			}

			// 返回 Json 存储的对象类型ID
			int type() const { return this->json_type; }

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
					case GetJsonTypeID<typename JsonType::JsonInt>::ID:
						return cstr(transform<typename JsonType::JsonInt>());
					case GetJsonTypeID<typename JsonType::JsonFloat>::ID:
						return cstr(transform<typename JsonType::JsonFloat>());
					case GetJsonTypeID<typename JsonType::JsonBool>::ID:
						return cstr(transform<typename JsonType::JsonBool>());
					case GetJsonTypeID<typename JsonType::JsonStr>::ID:
						return transform<typename JsonType::JsonStr>().__str__();
					case GetJsonTypeID<typename JsonType::JsonArray>::ID:
						return transform<typename JsonType::JsonArray>().__str__();
					case GetJsonTypeID<typename JsonType::JsonDict>::ID:
						return transform<typename JsonType::JsonDict>().__str__();
					case GetJsonTypeID<typename JsonType::JsonNull>::ID:
						return cstr("null");
				}
			}

			// 尾部添加一个Json对象，需要当前Json对象为JsonArray类型
			Json& append(const Json& json)
			{
				transform<typename JsonType::JsonArray>().append(json);
				return *this;
			}

			Json& operator[] (const typename JsonType::JsonStr& key)
			{
				return transform<typename JsonType::JsonDict>()[key];
			}

			const Json& operator[] (const typename JsonType::JsonStr& key) const
			{
				return transform<typename JsonType::JsonDict>().get(key);
			}

			Json& operator[] (size_t index)
			{
				return transform<typename JsonType::JsonArray>()[index];
			}

			const Json& operator[] (size_t index) const
			{
				return transform<typename JsonType::JsonArray>()[index];
			}

			void pop(size_t index = -1)
			{
				transform<typename JsonType::JsonArray>().pop(index);
			}

			size_t size() const
			{
				if (json_type == GetJsonTypeID<typename JsonType::JsonArray>::ID)
					return transform<typename JsonType::JsonArray>().size();

				if (json_type == GetJsonTypeID<typename JsonType::JsonDict>::ID)
					return transform<typename JsonType::JsonDict>().size();

				RuntimeError("Json type is not JSON_ARRAY or JSON_DICT");
			}

		private:
			// 释放内存
			void release()
			{
				switch (json_type)
				{
				case GetJsonTypeID<typename JsonType::JsonInt>::ID:
					delete transform_ptr<typename JsonType::JsonInt>();
					break;
				case GetJsonTypeID<typename JsonType::JsonFloat>::ID:
					delete transform_ptr<typename JsonType::JsonFloat>();
					break;
				case GetJsonTypeID<typename JsonType::JsonBool>::ID:
					delete transform_ptr<typename JsonType::JsonBool>();
					break;
				case GetJsonTypeID<typename JsonType::JsonStr>::ID:
					delete transform_ptr<typename JsonType::JsonStr>();
					break;
				case GetJsonTypeID<typename JsonType::JsonArray>::ID:
					delete transform_ptr<typename JsonType::JsonArray>();
					break;
				case GetJsonTypeID<typename JsonType::JsonDict>::ID:
					delete transform_ptr<typename JsonType::JsonDict>();
					break;
				case GetJsonTypeID<typename JsonType::JsonNull>::ID:
					delete transform_ptr<typename JsonType::JsonNull>();
					break;
				default:
					ValueError(std::format("json_type can not be {}"), type());
				}
			}

			// 检查类型是否正确
			template<JsonTypeStrictConcept T>
			void check_type() const
			{
				error_assert(json_type == GetJsonTypeID<T>::ID, 
					std::format("Json type is not {}\n", dtype(T))
				);
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

			RuntimeError(std::format("Unsupported {} for make int"), dtype(T));
			return None<JsonType::JsonInt>;
		}

		template<typename T>
		constexpr inline JsonType::JsonFloat make_float(T&& value)
		{
			using RT = std::remove_cvref_t<T>;

			if constexpr (std::is_arithmetic_v<RT>)
				return static_cast<JsonType::JsonFloat>(value);

			RuntimeError(std::format("Unsupported {} for make float"), dtype(T));
			return None<JsonType::JsonFloat>;
		}

		template<typename T>
		constexpr inline JsonType::JsonBool make_bool(T&& value)
		{
			return static_cast<JsonType::JsonBool>(value);
		}
	}
}
