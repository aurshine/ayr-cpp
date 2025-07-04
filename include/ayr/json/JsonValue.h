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

			int8_t json_type_id_;
		public:

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

				void for_null(std::function<R(NullArgT)>&& func) { for_null_ = std::move(func); }

				void for_int(std::function<R(IntArgT)>&& func) { for_int_ = std::move(func); }

				void for_float(std::function<R(FloatArgT)>&& func) { for_float_ = std::move(func); }

				void for_bool(std::function<R(BoolArgT)>&& func) { for_bool_ = std::move(func); }

				void for_str(std::function<R(StrArgT)>&& func) { for_str_ = std::move(func); }

				void for_array(std::function<R(ArrayArgT)>&& func) { for_array_ = std::move(func); }

				void for_dict(std::function<R(DictArgT)>&& func) { for_dict_ = std::move(func); }

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
			};

			template<typename R = void>
			using JsonMethods = JsonMethodsImpl<R, false>;

			template<typename R = void>
			using JsonMethodsC = JsonMethodsImpl<R, true>;

			Json() : json_item_(ayr_make<JsonNull>()), json_type_id_(GetJsonTypeID<JsonNull>::ID) {}

			template<JsonTypeConcept T>
			explicit Json(const T& value) : json_item_(ayr_make<T>(value)), json_type_id_(GetJsonTypeID<T>::ID) {}

			template<JsonTypeConcept T>
			explicit Json(T&& value) : json_item_(ayr_make<T>(std::forward<T>(value))), json_type_id_(GetJsonTypeID<T>::ID) {}

			Json(const Json& other) : json_item_(nullptr), json_type_id_(other.json_type_id_)
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
			}

			Json(Json&& other) : json_item_(other.json_item_), json_type_id_(other.json_type_id_)
			{
				other.json_item_ = nullptr;
				other.json_type_id_ = -1;
			}

			~Json()
			{
				if (this->json_type_id_ == -1) return;

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
				json_type_id_ = -1;
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
				return *ayr_construct(this, std::forward<T>(value));
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

			// 获取Json类型名称
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

			// 尾部添加一个Json对象，需要当前Json对象为JsonArray类型
			Json& append(const Json& json)
			{
				JsonMethods<Json&> jm;
				jm.for_array([&json](JsonArray& obj) -> Json& { return obj.append(json); });
				return jm(*this);
			}

			Json& operator[] (const JsonStr& key)
			{
				JsonMethods<Json&> jm;
				jm.for_dict([&key](JsonDict& obj) -> Json& { return obj[key]; });
				return jm(*this);
			}

			const Json& operator[] (const JsonStr& key) const
			{
				JsonMethodsC<const Json&> jmc;
				jmc.for_dict([&key](const JsonDict& obj) -> const Json& { return obj[key]; });
				return jmc(*this);
			}

			Json& operator[] (c_size index)
			{
				JsonMethods<Json&> jm;
				jm.for_array([&index](JsonArray& obj) -> Json& { return obj[index]; });
				return jm(*this);
			}

			const Json& operator[] (c_size index) const
			{
				JsonMethodsC<const Json&> jmc;
				jmc.for_array([&index](const JsonArray& obj) -> const Json& { return obj[index]; });
				return jmc(*this);
			}

			void pop(c_size index)
			{
				JsonMethods<> jm;
				jm.for_array([&index](JsonArray& obj) { obj.pop(index); });
				jm(*this);
			}

			void pop(const JsonStr& key)
			{
				JsonMethods<> jm;
				jm.for_dict([&key](JsonDict& obj) { obj.pop(key); });
				jm(*this);
			}

			void pop(const Json& json_obj)
			{
				JsonMethods<> jm;
				jm.for_array([&json_obj](JsonArray& obj) { obj.pop_if([&json_obj](const Json& json) { return json == json_obj; }); });
				jm(*this);
			}

			void clear()
			{
				JsonMethods<> jm;
				jm.for_array([](JsonArray& obj) { obj.clear(); });
				jm.for_dict([](JsonDict& obj) { obj.clear(); });
				jm(*this);
			}

			c_size size() const
			{
				JsonMethodsC<c_size> jmc;
				jmc.for_array([](const JsonArray& obj) { return obj.size(); });
				jmc.for_dict([](const JsonDict& obj) { return obj.size(); });
				return jmc(*this);
			}

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
				jmc.for_dict([&](const JsonDict& obj) { return cstr(obj); });
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
		};

		template<typename T>
		constexpr inline Json make_int(T&& value)
		{
			using RT = std::remove_cvref_t<T>;
			if constexpr (std::is_arithmetic_v<RT>)
				return Json(static_cast<JsonInt>(value));

			RuntimeError(std::format("Unsupported {} for make int", dtype(T)));
			return None<Json>;
		}

		template<typename T>
		constexpr inline Json make_float(T&& value)
		{
			using RT = std::remove_cvref_t<T>;
			if constexpr (std::is_arithmetic_v<RT>)
				return Json(static_cast<JsonFloat>(value));

			RuntimeError(std::format("Unsupported {} for make float", dtype(T)));
			return None<Json>;
		}

		template<typename T>
		constexpr inline Json make_bool(T&& value)
		{
			return Json(static_cast<JsonBool>(value));
		}
	}
}
#endif  AYR_JSON_JSONVALUE_H