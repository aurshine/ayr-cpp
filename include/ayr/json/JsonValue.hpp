#ifndef AYR_JSON_JSONVALUE_HPP
#define AYR_JSON_JSONVALUE_HPP

#include <variant>

#include "../air/Dict.hpp"
#include "../air/DynArray.hpp"

namespace ayr
{
	namespace json
	{
		class Json;

		using JsonNull = std::monostate;

		using JsonInt = c_size;

		using JsonFloat = double;

		using JsonBool = bool;

		using JsonStr = Atring;

		using JsonArray = DynArray<Json>;

		using JsonDict = Dict<JsonStr, Json>;

		// JSON解析和转储的默认最大容器嵌套深度
		static constexpr c_size DEFAULT_MAX_DEPTH = 128;

		template<typename T>
		concept JsonTypeConcept = issame<T, JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, JsonArray, JsonDict>;

		/*
		* @brief 可以直接构造为Json的值类型
		*
		* @details 除Json实际存储的类型外，普通整数和浮点数也会分别归一化为JsonInt和JsonFloat
		*/
		template<typename T>
		concept JsonValueConcept = JsonTypeConcept<T> ||
			std::integral<std::remove_cvref_t<T>> ||
			std::floating_point<std::remove_cvref_t<T>>;

		template<typename T>
		struct _IsJsonLike : std::false_type {};

		template<>
		struct _IsJsonLike<Json> : std::true_type {};

		template<>
		struct _IsJsonLike<JsonNull> : std::true_type {};

		template<>
		struct _IsJsonLike<JsonInt> : std::true_type {};

		template<>
		struct _IsJsonLike<JsonFloat> : std::true_type {};

		template<>
		struct _IsJsonLike<JsonBool> : std::true_type {};

		template<>
		struct _IsJsonLike<JsonStr> : std::true_type {};

		template<typename T>
		struct _IsJsonLike<DynArray<T>> : std::bool_constant<_IsJsonLike<T>::value> {};

		template<typename T>
		struct _IsJsonLike<Dict<JsonStr, T>> : std::bool_constant<_IsJsonLike<T>::value> {};

		/*
		* @brief 近似Json的概念
		*
		* @details Json, JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, DynArray<JsonLike>, Dict<JsonStr, JsonLike>均符合该概念
		*/
		template<typename T>
		concept JsonLikeConcept = _IsJsonLike<std::decay_t<T>>::value;

		template<JsonLikeConcept T>
		bool sample_type(const T& obj)
		{
			if constexpr (issame<T, JsonArray, JsonDict>)
				return false;
			if constexpr (issame<T, Json>)
				if (obj.template is<JsonArray>() || obj.template is<JsonDict>())
					return false;
			return true;
		}

		/*
		* @brief JSON值类型
		* 
		* @details 支持Null, Int, Float, Bool, Str, Array, Dict类型
		*/
		class Json
		{
			using self = Json;

			using variant_type = std::variant<JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, JsonArray, JsonDict>;

			variant_type json_var_;

			template<JsonValueConcept T>
			static constexpr variant_type make_variant(T&& value)
			{
				using value_type = std::remove_cvref_t<T>;
				if constexpr (issame<value_type, JsonBool>)
					return variant_type(std::in_place_type<JsonBool>, static_cast<JsonBool>(value));
				else if constexpr (std::integral<value_type>)
					return variant_type(std::in_place_type<JsonInt>, static_cast<JsonInt>(value));
				else if constexpr (std::floating_point<value_type>)
					return variant_type(std::in_place_type<JsonFloat>, static_cast<JsonFloat>(value));
				else
					return variant_type(std::in_place_type<value_type>, std::forward<T>(value));
			}
		public:
			constexpr Json() : json_var_(JsonNull()) {}

			template<JsonValueConcept T>
			constexpr Json(T&& value) : json_var_(make_variant(std::forward<T>(value))) {}

			constexpr Json(const Json& other) : json_var_(other.json_var_) {}

			constexpr Json(Json&& other) noexcept : json_var_(std::move(other.json_var_)) {}

			template<JsonValueConcept T>
			constexpr self& operator=(T&& value)
			{
				json_var_ = make_variant(std::forward<T>(value));
				return *this;
			}

			constexpr self& operator=(const self& other)
			{
				json_var_ = other.json_var_;
				return *this;
			}

			constexpr self& operator=(self&& other) noexcept
			{
				json_var_ = std::move(other.json_var_);
				return *this;
			}

			template<JsonTypeConcept T>
			constexpr bool is() const { return std::holds_alternative<T>(json_var_); }

			template<JsonTypeConcept T>
			constexpr T& as() { return std::get<T>(json_var_); }

			template<JsonTypeConcept T>
			constexpr const T& as() const { return std::get<T>(json_var_);  }

			// 访问json的实际元素
			template<typename Callable>
			auto visit(Callable&& fn) { return std::visit(std::forward<Callable>(fn), json_var_); }

			// 访问json的实际元素
			template<typename Callable>
			auto visit(Callable&& fn) const { return std::visit(std::forward<Callable>(fn), json_var_); }

			/*
			* @brief 获取Json类型名称
			*
			* @return CString 类型名称
			*/
			constexpr CString type_name() const
			{
				return visit([](auto&& v) {
					using T = decltype(v);
					if constexpr (issame<T, JsonNull>)
						return "Null";
					else if constexpr (issame<T, JsonInt>)
						return "Int";
					else if constexpr (issame<T, JsonBool>)
						return "Bool";
					else if constexpr (issame<T, JsonFloat>)
						return "Float";
					else if constexpr (issame<T, JsonStr>)
						return "Str";
					else if constexpr (issame<T, JsonArray>)
						return "Array";
					else if constexpr (issame<T, JsonDict>)
						return "Dict";
					});
			}

			bool contains(const Json& obj) const
			{
				if (is<JsonArray>())
					return as<JsonArray>().contains(obj);
				return func_call_error("contains()");
			}

			bool contains(const JsonStr& obj) const
			{
				if (is<JsonArray>())
					return contains(Json(obj));
				else if (is<JsonDict>())
					return as<JsonDict>().contains(obj);
				return func_call_error("contains()");
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
			Json& append(Json json)
			{
				if (is<JsonArray>())
					return as<JsonArray>().append(std::move(json));
				return func_call_error("append(json)");
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
			Json& operator[] (JsonStr key)
			{
				if (is<JsonDict>())
					return as<JsonDict>()[std::move(key)];
				return func_call_error("operator[key]");
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
			const Json& operator[] (JsonStr key) const
			{
				if (is<JsonDict>())
					return as<JsonDict>()[std::move(key)];
				return func_call_error("operator[key]");
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
				if (is<JsonArray>())
					return as<JsonArray>()[index];
				return func_call_error("operator[index]");;
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
				if (is<JsonArray>())
					return as<JsonArray>()[index];
				return func_call_error("operator[index]");
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
				if (is<JsonArray>())
					return as<JsonArray>().pop(index);
				func_call_error("pop(index)");
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
				if (is<JsonDict>())
					return as<JsonDict>().pop(key);
				func_call_error("pop(key)");
			}

			/*
			* @brief 清空Json对象内部的值
			*
			* @warning 若当前Json对象不是JsonArray或JsonDict类型，会抛出JSON_TYPE_INVALID_ERROR异常
			*/
			void clear()
			{
				return visit([&](auto&& v) {
					using T = decltype(v);
					if constexpr (issame<T, JsonArray, JsonDict>)
						return v.clear();
					func_call_error("clear");
					});
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
				return visit([&](auto&& v) -> c_size {
					using T = decltype(v);
					if constexpr (issame<T, JsonStr, JsonArray, JsonDict>)
						return v.size();
					return func_call_error("size");
					});
			}

			c_size empty() const { return size() == 0; }

			bool operator==(const Json& other) const { return json_var_ == other.json_var_; }

			void __repr__(Buffer& buffer) const;
		private:
			decltype(None) func_call_error(const CString& fname) const
			{
				JsonValueError(ayr::format("type {} cannot call {}", type_name(), fname));
				return None;
			}
		};

		template<typename T>
			requires std::is_arithmetic_v<T>
		def integer(const T& member) -> Json{ return static_cast<JsonInt>(member); }

		template<typename T>
			requires std::is_arithmetic_v<T>
		def floating(const T& member) -> Json { return static_cast<JsonFloat>(member); }

		template<typename T>
			requires std::is_arithmetic_v<T>
		def boolean(const T& member) -> Json { return static_cast<JsonBool>(member); }

		def array(const std::initializer_list<Json>& members = {}) -> Json
		{
			JsonArray arr;
			for (const Json& member : members)
				arr.append(member);
			return Json(std::move(arr));
		}

		def dict(const std::initializer_list<std::pair<JsonStr, Json>>& members = {}) -> Json
		{
			JsonDict d;
			for (const std::pair<JsonStr, Json>& member : members)
				d.insert(member.first, member.second);
			return Json(std::move(d));
		}
	}
}
#endif // AYR_JSON_JSONVALUE_HPP
