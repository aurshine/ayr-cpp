#ifndef AYR_JSON_JSONVALUE_H
#define AYR_JSON_JSONVALUE_H

#include <variant>

#include "../air/Dict.hpp"
#include "../air/DynArray.hpp"

namespace ayr
{
	namespace json
	{
		class Json;

		using JsonNull = std::monostate;

		using JsonInt = long long;

		using JsonFloat = double;

		using JsonBool = bool;

		using JsonStr = Atring;

		using JsonArray = DynArray<Json>;

		using JsonDict = Dict<JsonStr, Json>;


		template<typename T>
		concept JsonTypeConcept = issame<T, JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, JsonArray, JsonDict>;

		class Json
		{
			using self = Json;

			std::variant<JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, JsonArray, JsonDict> json_var_;
		public:
			constexpr Json() : json_var_(JsonNull()) {}

			template<JsonTypeConcept T>
			constexpr explicit Json(T&& value) : json_var_(std::forward<T>(value)) {}

			constexpr Json(const Json& other) : json_var_(other.json_var_) {}

			constexpr Json(Json&& other) noexcept : json_var_(std::move(other.json_var_)) {}

			template<JsonTypeConcept T>
			constexpr self& operator=(const T& value)
			{
				json_var_.emplace(value);
				return *this;
			}

			template<JsonTypeConcept T>
			constexpr self& operator=(T&& value)
			{
				json_var_.emplace(std::move(value));
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

			constexpr bool is_null() const { return std::holds_alternative<JsonNull>(json_var_); }

			constexpr bool is_int() const { return std::holds_alternative<JsonInt>(json_var_); }

			constexpr bool is_float() const { return std::holds_alternative<JsonFloat>(json_var_); }

			constexpr bool is_bool() const { return std::holds_alternative<JsonBool>(json_var_); }

			constexpr bool is_str() const { return std::holds_alternative<JsonStr>(json_var_); }

			constexpr bool is_array() const { return std::holds_alternative<JsonArray>(json_var_); }

			constexpr bool is_dict() const { return std::holds_alternative<JsonDict>(json_var_); }

			constexpr JsonNull& as_null() { return std::get<JsonNull>(json_var_); }

			constexpr const JsonNull& as_null() const { return std::get<JsonNull>(json_var_); }

			constexpr JsonInt& as_int() { return std::get<JsonInt>(json_var_); }

			constexpr const JsonInt& as_int() const { return std::get<JsonInt>(json_var_); }

			constexpr JsonFloat& as_float() { return std::get<JsonFloat>(json_var_); }

			constexpr const JsonFloat& as_float() const { return std::get<JsonFloat>(json_var_); }

			constexpr JsonBool& as_bool() { return std::get<JsonBool>(json_var_); }

			constexpr const JsonBool& as_bool() const { return std::get<JsonBool>(json_var_); }

			constexpr JsonStr& as_str() { return std::get<JsonStr>(json_var_); }

			constexpr const JsonStr& as_str() const { return std::get<JsonStr>(json_var_); }

			constexpr JsonArray& as_array() { return std::get<JsonArray>(json_var_); }

			constexpr const JsonArray& as_array() const { return std::get<JsonArray>(json_var_); }

			constexpr JsonDict& as_dict() { return std::get<JsonDict>(json_var_); }

			constexpr const JsonDict& as_dict() const { return std::get<JsonDict>(json_var_); }

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
				if (is_array())
					return as_array().append(json);
				JsonValueError(std::format("type {} cannot call append", type_name()));
				return None;
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
				if (is_dict())
					return as_dict()[key];
				JsonValueError(std::format("type {} cannot call operator[key]", type_name()));
				return None;
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
				if (is_dict())
					return as_dict()[key];
				JsonValueError(std::format("type {} cannot call operator[key]", type_name()));
				return None;
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
				if (is_array())
					return as_array()[index];
				JsonValueError(std::format("type {} cannot call operator[index]", type_name()));
				return None;
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
				if (is_array())
					return as_array()[index];
				JsonValueError(std::format("type {} cannot call operator[index]", type_name()));
				return None;
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
				if (is_array())
					as_array().pop(index);
				JsonValueError(std::format("type {} cannot call pop(index)", type_name()));
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
				if (is_dict())
					as_dict().pop(key);
				JsonValueError(std::format("type {} cannot call pop(key)", type_name()));
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
					JsonValueError(std::format("type {} cannot call clear", type_name()));
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
					JsonValueError(std::format("type {} cannot call clear", type_name()));
					return None;
					});
			}

			// 访问json的实际元素
			template<typename Callable>
			auto visit(Callable&& fn) { return std::visit(std::forward<Callable>(fn), json_var_); }

			// 访问json的实际元素
			template<typename Callable>
			auto visit(Callable&& fn) const { return std::visit(std::forward<Callable>(fn), json_var_); }

			bool operator==(const Json& other) const { return json_var_ == other.json_var_; }

			void __repr__(Buffer& buffer) const { fmt_buf(buffer, 0, "    "); }
		private:
			// 格式化输出json对象
			void fmt_buf(Buffer& buffer, int depth, const CString& indent) const
			{
				struct Visitor
				{
					Buffer& vb;

					int depth;

					const CString& indent;

					void operator()(const JsonNull& obj) { vb << "null"; }

					void operator()(const JsonInt& obj) { vb << obj; }

					void operator()(const JsonFloat& obj) { vb << obj; }

					void operator()(const JsonBool& obj) { vb << obj; }

					void operator()(const JsonStr& obj) { vb << "\"" << obj << "\""; }

					void operator()(const JsonArray& obj)
					{
						vb << "[";
						bool is_ds = any(obj, [](const Json& item) {
							return item.is_array() || item.is_dict();
							});

						if (is_ds) fmt_new_line(vb, depth + 1, indent);

						bool flag = false;
						for (auto& item : obj)
						{
							if (flag)
							{
								vb << ", ";
								if (is_ds) fmt_new_line(vb, depth + 1, indent);
							}
							flag = true;
							item.fmt_buf(vb, depth + 1, indent);
						}

						if (is_ds) fmt_new_line(vb, depth, indent);

						vb << "]";
					}

					void operator()(const JsonDict& obj)
					{
						vb << "{";
						bool is_ds = any(obj.values(), [](const Json& item) {
							return item.is_array() || item.is_dict();
							});

						if (is_ds) fmt_new_line(vb, depth + 1, indent);

						bool flag = false;
						for (auto& [k, v] : obj.items())
						{
							if (flag)
							{
								vb << ", ";
								if (is_ds) fmt_new_line(vb, depth + 1, indent);
							}
							flag = true;
							vb << "\"" << k << "\": ";
							v.fmt_buf(vb, depth + 1, indent);
						}

						if (is_ds) fmt_new_line(vb, depth, indent);
						vb << "}";
					}

					// 格式化输出换行
					void fmt_new_line(Buffer& buffer, int depth, const CString& indent) const
					{
						buffer << "\n";
						while (depth--)
							buffer << indent;
					}
				};
				visit(Visitor{ buffer, depth, indent });
			}
		};
	}
}
#endif  AYR_JSON_JSONVALUE_H