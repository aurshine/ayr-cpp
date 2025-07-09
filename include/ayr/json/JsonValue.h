#ifndef AYR_JSON_JSONVALUE_H
#define AYR_JSON_JSONVALUE_H

#include "json_base.hpp"

namespace ayr
{
	namespace json
	{
		class Json : public Object<Json>
		{
			using self = Json;

			using super = Object<Json>;

			void* json_item_;

			// json对象的类型id
			int8_t json_type_id_;
		public:

			/*
			* @brief 根据json_type_id 将json对象分发到对应的处理函数中
			*
			* @detail
			*
			* R 模板参数表示处理函数的返回值
			*
			* IsConst 表示处理函数的参数是否传入const对象
			*/
			template<typename R, bool IsConst = false>
			class JsonMethodsImpl : public Object<JsonMethodsImpl<R, IsConst>>
			{
				using self = JsonMethodsImpl;

				using super = Object<self>;

				using NullArgT = add_const_t<IsConst, JsonNull>&;

				using IntArgT = add_const_t<IsConst, JsonInt>&;

				using FloatArgT = add_const_t<IsConst, JsonFloat>&;

				using BoolArgT = add_const_t<IsConst, JsonBool>&;

				using StrArgT = add_const_t<IsConst, JsonStr>&;

				using ArrayArgT = add_const_t<IsConst, JsonArray>&;

				using DictArgT = add_const_t<IsConst, JsonDict>&;

				std::function<R(NullArgT)> for_null_;

				std::function<R(IntArgT)> for_int_;

				std::function<R(FloatArgT)> for_float_;

				std::function<R(BoolArgT)> for_bool_;

				std::function<R(StrArgT)> for_str_;

				std::function<R(ArrayArgT)> for_array_;

				std::function<R(DictArgT)> for_dict_;
			public:
				JsonMethodsImpl() {}

				void for_null(std::function<R(NullArgT)> func) { for_null_ = std::move(func); }

				void for_int(std::function<R(IntArgT)> func) { for_int_ = std::move(func); }

				void for_float(std::function<R(FloatArgT)> func) { for_float_ = std::move(func); }

				void for_bool(std::function<R(BoolArgT)> func) { for_bool_ = std::move(func); }

				void for_str(std::function<R(StrArgT)> func) { for_str_ = std::move(func); }

				void for_array(std::function<R(ArrayArgT)> func) { for_array_ = std::move(func); }

				void for_dict(std::function<R(DictArgT)> func) { for_dict_ = std::move(func); }

				/*
				* @brief 根据传入的json对象的类型id，转发到对应的处理函数中
				*
				* @param obj 传入的json对象
				*
				* @return R 返回处理函数的返回值
				*
				* @warning 若处理函数没有被定义，会抛出JSON_TYPE_INVALID_ERROR异常
				*/
				R operator()(add_const_t<IsConst, Json>& obj) const
				{
					if (obj.is_null())
						return null_fn(obj.as_null());
					else if (obj.is_int())
						return int_fn(obj.as_int());
					else if (obj.is_float())
						return float_fn(obj.as_float());
					else if (obj.is_bool())
						return bool_fn(obj.as_bool());
					else if (obj.is_str())
						return str_fn(obj.as_str());
					else if (obj.is_array())
						return array_fn(obj.as_array());
					else if (obj.is_dict())
						return dict_fn(obj.as_dict());

					JSON_TYPE_INVALID_ERROR;
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}
			private:
				R null_fn(NullArgT obj) const
				{
					if (for_null_) return for_null_(obj);
					json_invalid_error<JsonNull>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R int_fn(IntArgT obj) const
				{
					if (for_int_) return for_int_(obj);
					json_invalid_error<JsonInt>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R float_fn(FloatArgT obj) const
				{
					if (for_float_) return for_float_(obj);
					json_invalid_error<JsonFloat>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R bool_fn(BoolArgT obj) const
				{
					if (for_bool_) return for_bool_(obj);
					json_invalid_error<JsonBool>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R str_fn(StrArgT obj) const
				{
					if (for_str_) return for_str_(obj);
					json_invalid_error<JsonStr>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R array_fn(ArrayArgT obj) const
				{
					if (for_array_) return for_array_(obj);
					json_invalid_error<JsonArray>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}

				R dict_fn(DictArgT obj) const
				{
					if (for_dict_) return for_dict_(obj);
					json_invalid_error<JsonDict>();
					if constexpr (Not<issame<R, void>>)
						return None<R>;
				}
			};

			template<typename R = void>
			using JsonMethods = JsonMethodsImpl<R, false>;

			template<typename R = void>
			using JsonMethodsC = JsonMethodsImpl<R, true>;

			Json() : json_item_(ayr_make<JsonNull>()), json_type_id_(GetJsonTypeID<JsonNull>::ID) {}

			template<JsonTypeConcept T>
			explicit Json(const T& value) : json_item_(ayr_make<T>(value)), json_type_id_(GetJsonTypeID<T>::ID) {}

			template<JsonTypeConcept T>
			explicit Json(T&& value) : json_item_(ayr_make<T>(std::move(value))), json_type_id_(GetJsonTypeID<T>::ID) {}

			Json(const Json& other) : json_item_(nullptr), json_type_id_(INVALID_JSON_TYPE_ID)
			{
				JsonMethodsC<> jmc;
				jmc.for_null([&](const JsonNull& obj) { ayr_construct(this, obj); });
				jmc.for_int([&](const JsonInt& obj) { ayr_construct(this, obj); });
				jmc.for_float([&](const JsonFloat& obj) { ayr_construct(this, obj); });
				jmc.for_bool([&](const JsonBool& obj) { ayr_construct(this, obj); });
				jmc.for_str([&](const JsonStr& obj) { ayr_construct(this, obj); });
				jmc.for_array([&](const JsonArray& obj) { ayr_construct(this, obj); });
				jmc.for_dict([&](const JsonDict& obj) { ayr_construct(this, obj); });
				jmc(other);
				json_type_id_ = other.json_type_id_;
			}

			Json(Json&& other) noexcept : json_item_(other.json_item_), json_type_id_(other.json_type_id_)
			{
				other.json_item_ = nullptr;
				other.json_type_id_ = INVALID_JSON_TYPE_ID;
			}

			~Json()
			{
				if (this->json_type_id_ == INVALID_JSON_TYPE_ID) return;

				JsonMethods<> jm;
				jm.for_null([&](JsonNull& obj) { ayr_desloc(&obj); });
				jm.for_int([&](JsonInt& obj) { ayr_desloc(&obj); });
				jm.for_float([&](JsonFloat& obj) { ayr_desloc(&obj); });
				jm.for_bool([&](JsonBool& obj) { ayr_desloc(&obj); });
				jm.for_str([&](JsonStr& obj) { ayr_desloc(&obj); });
				jm.for_array([&](JsonArray& obj) { ayr_desloc(&obj); });
				jm.for_dict([&](JsonDict& obj) { ayr_desloc(&obj); });
				jm(*this);
				json_item_ = nullptr;
				json_type_id_ = INVALID_JSON_TYPE_ID;
			}

			template<JsonTypeConcept T>
			self& operator=(const T& value)
			{
				ayr_destroy(this);
				return *ayr_construct(this, value);
			}

			template<JsonTypeConcept T>
			self& operator=(T&& value)
			{
				ayr_destroy(this);
				return *ayr_construct(this, std::move(value));
			}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, other);
			}

			self& operator=(self&& other) noexcept
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, std::move(other));
			}

			bool is_null() const { return json_type_id_ == GetJsonTypeID<JsonNull>::ID; }

			bool is_int() const { return json_type_id_ == GetJsonTypeID<JsonInt>::ID; }

			bool is_float() const { return json_type_id_ == GetJsonTypeID<JsonFloat>::ID; }

			bool is_bool() const { return json_type_id_ == GetJsonTypeID<JsonBool>::ID; }

			bool is_str() const { return json_type_id_ == GetJsonTypeID<JsonStr>::ID; }

			bool is_array() const { return json_type_id_ == GetJsonTypeID<JsonArray>::ID; }

			bool is_dict() const { return json_type_id_ == GetJsonTypeID<JsonDict>::ID; }

			JsonNull& as_null() { return *reinterpret_cast<JsonNull*>(json_item_); }

			const JsonNull& as_null() const { return *reinterpret_cast<const JsonNull*>(json_item_); }

			JsonInt& as_int() { return *reinterpret_cast<JsonInt*>(json_item_); }

			const JsonInt& as_int() const { return *reinterpret_cast<const JsonInt*>(json_item_); }

			JsonFloat& as_float() { return *reinterpret_cast<JsonFloat*>(json_item_); }

			const JsonFloat& as_float() const { return *reinterpret_cast<const JsonFloat*>(json_item_); }

			JsonBool& as_bool() { return *reinterpret_cast<JsonBool*>(json_item_); }

			const JsonBool& as_bool() const { return *reinterpret_cast<const JsonBool*>(json_item_); }

			JsonStr& as_str() { return *reinterpret_cast<JsonStr*>(json_item_); }

			const JsonStr& as_str() const { return *reinterpret_cast<const JsonStr*>(json_item_); }

			JsonArray& as_array() { return *reinterpret_cast<JsonArray*>(json_item_); }

			const JsonArray& as_array() const { return *reinterpret_cast<const JsonArray*>(json_item_); }

			JsonDict& as_dict() { return *reinterpret_cast<JsonDict*>(json_item_); }

			const JsonDict& as_dict() const { return *reinterpret_cast<const JsonDict*>(json_item_); }

			/*
			* @brief 获取Json类型名称
			*
			* @return CString 类型名称
			*/
			CString type_name() const
			{
				JsonMethodsC<CString> jmc;
				jmc.for_null([&](const JsonNull&) -> CString { return "null"; });
				jmc.for_int([&](const JsonInt&) -> CString { return "int"; });
				jmc.for_float([&](const JsonFloat&) -> CString { return "float"; });
				jmc.for_bool([&](const JsonBool&) -> CString { return "bool"; });
				jmc.for_str([&](const JsonStr&) -> CString { return "str"; });
				jmc.for_array([&](const JsonArray&) -> CString { return "array"; });
				jmc.for_dict([&](const JsonDict&) -> CString { return "dict"; });
				return jmc(*this);
			}

			/*
			* @brief 尾部添加一个Json对象
			*
			* @param json 待添加的Json对象
			*
			* @return Json& 当前被添加的Json对象
			*
			* @warning 若当前Json对象不是JsonArray类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			Json& append(const Json& json)
			{
				JsonMethods<Json&> jm;
				jm.for_array([&json](JsonArray& obj) -> Json& { return obj.append(json); });
				return jm(*this);
			}

			/*
			* @brief 根据key获取Json对象
			*
			* @detail
			* 若不存在key，则会自动创建并返回一个空Json对象
			*
			* @param key 待获取的key
			*
			* @return Json& 对应key的Json对象
			*
			* @warning 若当前Json对象不是JsonDict类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			Json& operator[] (const JsonStr& key)
			{
				JsonMethods<Json&> jm;
				jm.for_dict([&key](JsonDict& obj) -> Json& { return obj[key]; });
				return jm(*this);
			}

			/*
			* @brief 根据key获取Json对象
			*
			* @detail
			* 若不存在key，则会自动创建并返回一个空Json对象
			*
			* @param key 待获取的key
			*
			* @return Json& 对应key的Json对象
			*
			* @warning 若当前Json对象不是JsonDict类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			const Json& operator[] (const JsonStr& key) const
			{
				JsonMethodsC<const Json&> jmc;
				jmc.for_dict([&key](const JsonDict& obj) -> const Json& { return obj[key]; });
				return jmc(*this);
			}

			/*
			* @brief 根据下标获取Json对象
			*
			* @detail
			* 下标可以为负数，表示从尾部开始的下标
			*
			* @param index 待获取的下标
			*
			* @return Json& 对应下标的Json对象
			*
			* @warning 若当前Json对象不是JsonArray类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			Json& operator[] (c_size index)
			{
				JsonMethods<Json&> jm;
				jm.for_array([&index](JsonArray& obj) -> Json& { return obj[index]; });
				return jm(*this);
			}

			/*
			* @brief 根据下标获取Json对象
			*
			* @detail
			* 下标可以为负数，表示从尾部开始的下标
			*
			* @param index 待获取的下标
			*
			* @return Json& 对应下标的Json对象
			*
			* @warning 若当前Json对象不是JsonArray类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			const Json& operator[] (c_size index) const
			{
				JsonMethodsC<const Json&> jmc;
				jmc.for_array([&index](const JsonArray& obj) -> const Json& { return obj[index]; });
				return jmc(*this);
			}

			/*
			* @brief 根据下标删除Json对象
			*
			* @detail
			* 下标可以为负数，表示从尾部开始的下标
			*
			* @param index 待删除的下标
			*
			* @warning 若当前Json对象不是JsonArray类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			void pop(c_size index)
			{
				JsonMethods<> jm;
				jm.for_array([&index](JsonArray& obj) { obj.pop(index); });
				jm(*this);
			}

			/*
			* @brief 根据key删除Json对象
			*
			* @param key 待删除的key
			*
			* @warning 若当前Json对象不是JsonDict类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			void pop(const JsonStr& key)
			{
				JsonMethods<> jm;
				jm.for_dict([&key](JsonDict& obj) { obj.pop(key); });
				jm(*this);
			}

			/*void pop(const Json& json_obj)
			{
				JsonMethods<> jm;
				jm.for_array([&json_obj](JsonArray& obj) { obj.pop_if([&json_obj](const Json& json) { return json == json_obj; }); });
				jm(*this);
			}*/

			/*
			* @brief 清空Json对象内部的值
			*
			* @warning 若当前Json对象不是JsonArray或JsonDict类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			void clear()
			{
				JsonMethods<> jm;
				jm.for_array([](JsonArray& obj) { obj.clear(); });
				jm.for_dict([](JsonDict& obj) { obj.clear(); });
				jm(*this);
			}

			/*
			* @brief 获取Json对象的大小
			*
			* @detail
			* - 对于JsonStr，返回字符串的长度
			* - 对于JsonArray，返回数组的元素个数
			* - 对于JsonDict，返回键值对的个数
			*
			* @return c_size Json对象的大小
			*
			* @warning 若当前Json对象不是JsonArray，JsonDict，JsonStr类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			c_size size() const
			{
				JsonMethodsC<c_size> jmc;
				jmc.for_array([](const JsonArray& obj) { return obj.size(); });
				jmc.for_dict([](const JsonDict& obj) { return obj.size(); });
				jmc.for_str([](const JsonStr& obj) { return obj.size(); });
				return jmc(*this);
			}

			void __repr__(Buffer& buffer) const { fmt_buf(buffer, 0, "    "); }

			// 转换为字符串
			CString __str__() const
			{
				JsonMethodsC<CString> jmc;
				jmc.for_null([&](const JsonNull&) { return "null"; });
				jmc.for_int([&](const JsonInt& obj) { return cstr(obj); });
				jmc.for_float([&](const JsonFloat& obj) { return cstr(obj); });
				jmc.for_bool([&](const JsonBool& obj) { return cstr(obj); });
				jmc.for_str([&](const JsonStr& obj) { return CString::cjoin(arr("\"", cstr(obj), "\"")); });
				jmc.for_array([&](const JsonArray& obj) { return cstr(obj); });
				jmc.for_dict([&](const JsonDict& obj)
					{
						DynArray<CString> strs;
						strs.append("{");
						for (auto& [key, value] : obj.items())
						{
							strs.append(CString::cjoin(arr("\"", cstr(key), "\"")));
							strs.append(": ");
							strs.append(cstr(value));
							strs.append(", ");
						}
						if (obj.size() > 0) strs.pop_back();

						strs.append("}");
						return CString::cjoin(strs);
					});
				return jmc(*this);
			}

			bool __equals__(const Json& other) const
			{
				if (this == &other) return true;
				if (json_type_id_ != other.json_type_id_) return false;
				JsonMethodsC<bool> jmc;
				jmc.for_null([&other](const JsonNull&) { return true; });
				jmc.for_int([&other](const JsonInt& obj) { return obj == other.as_int(); });
				jmc.for_float([&other](const JsonFloat& obj) { return obj == other.as_float(); });
				jmc.for_bool([&other](const JsonBool& obj) { return obj == other.as_bool(); });
				jmc.for_str([&other](const JsonStr& obj) { return obj == other.as_str(); });
				jmc.for_array([&other](const JsonArray& obj) { return obj == other.as_array(); });
				jmc.for_dict([&other](const JsonDict& obj) { return obj == other.as_dict(); });
				return jmc(*this);
			}

			void __swap__(self& other)
			{
				swap(json_item_, other.json_item_);
				swap(json_type_id_, other.json_type_id_);
			}

		private:
			// 格式化输出json对象
			void fmt_buf(Buffer& buffer, int depth, const CString& indent) const
			{
				JsonMethodsC<> jmc;

				jmc.for_null([&buffer](const JsonNull& obj) { buffer << "null"; });
				jmc.for_int([&buffer](const JsonInt& obj) { buffer << obj; });
				jmc.for_float([&buffer](const JsonFloat& obj) { buffer << obj; });
				jmc.for_bool([&buffer](const JsonBool& obj) { buffer << obj; });
				jmc.for_str([&buffer](const JsonStr& obj) { buffer << "\"" << obj << "\""; });

				jmc.for_array([&](const JsonArray& obj) {
					buffer << "[";
					bool is_ds = any(obj, [](const Json& item) {
						return item.is_array() || item.is_dict();
						});

					if (is_ds) fmt_new_line(buffer, depth + 1, indent);

					bool flag = false;
					for (auto& item : obj)
					{
						if (flag)
						{
							buffer << ", ";
							if (is_ds) fmt_new_line(buffer, depth + 1, indent);
						}
						flag = true;
						item.fmt_buf(buffer, depth + 1, indent);
					}

					if (is_ds) fmt_new_line(buffer, depth, indent);

					buffer << "]";
					});

				jmc.for_dict([&](const JsonDict& obj) {
					buffer << "{";
					bool is_ds = any(obj.values(), [](const Json& item) {
						return item.is_array() || item.is_dict();
						});

					if (is_ds) fmt_new_line(buffer, depth + 1, indent);

					bool flag = false;
					for (auto& [k, v] : obj.items())
					{
						if (flag)
						{
							buffer << ", ";
							if (is_ds) fmt_new_line(buffer, depth + 1, indent);
						}
						flag = true;
						buffer << "\"" << k << "\": ";
						v.fmt_buf(buffer, depth + 1, indent);
					}

					if (is_ds) fmt_new_line(buffer, depth, indent);
					buffer << "}";
					});

				jmc(*this);
			}

			// 格式化输出换行
			void fmt_new_line(Buffer& buffer, int depth, const CString& indent) const
			{
				buffer << "\n";
				while (depth--)
					buffer << indent;
			}
		};

		template<typename T>
		constexpr inline Json make_int(T&& value)
		{
			static_assert(std::is_arithmetic_v<std::remove_cvref_t<T>>,
				"Unsupported type for make int");

			return Json(static_cast<JsonInt>(value));
		}

		template<typename T>
		constexpr inline Json make_float(T&& value)
		{
			static_assert(std::is_arithmetic_v<std::remove_cvref_t<T>>,
				"Unsupported type for make int");

			return Json(static_cast<JsonFloat>(value));
		}

		template<typename T>
		constexpr inline Json make_bool(T&& value)
		{
			return Json(static_cast<JsonBool>(value));
		}
	}
}
#endif  AYR_JSON_JSONVALUE_H