#pragma once
#include <concepts>
#include <type_traits>

namespace ayr
{
	namespace json
	{
		class Json;
		class Null {};

		// 定义Json类型枚举
		struct JsonType
		{
			using JsonInt = long long;

			using JsonFloat = double;

			using JsonBool = bool;

			using JsonStr = std::string;

			using JsonArray = std::vector<Json>;

			using JsonDict = std::map<JsonStr, Json>;

			using JsonNull = Null;
		};


		// 严格类型概念
		template<typename T>
		concept JsonTypeStrictConcept = std::is_same_v<T, typename JsonType::JsonInt>
			|| std::is_same_v<T, typename JsonType::JsonFloat>
			|| std::is_same_v<T, typename JsonType::JsonBool>
			|| std::is_same_v<T, typename JsonType::JsonStr>
			|| std::is_same_v<T, typename JsonType::JsonArray>
			|| std::is_same_v<T, typename JsonType::JsonDict>
			|| std::is_same_v<T, typename JsonType::JsonNull>;

		// 可接受const reference volatile 修饰符的概念
		template<typename T>
		concept JsonTypeConcept = JsonTypeStrictConcept<std::remove_cvref_t<T>>;

		// 严格类型获得Json类型的枚举值
		template<JsonTypeStrictConcept T> struct GetJsonTypeEnumStrict { constexpr static int8_t ID = -1; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonInt> { constexpr static int8_t ID = 0; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonFloat> { constexpr static int8_t ID = 1; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonBool> { constexpr static int8_t ID = 2; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonStr> { constexpr static int8_t ID = 3; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonArray> { constexpr static int8_t ID = 4; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonDict> { constexpr static int8_t ID = 5; };
		template<> struct GetJsonTypeEnumStrict<typename JsonType::JsonNull> { constexpr static int8_t ID = 6; };

		// 可接受const reference volatile 修饰符获得Json类型的枚举值
		template<JsonTypeConcept T>
		struct GetJsonTypeEnum { constexpr static int8_t ID = GetJsonTypeEnumStrict<std::remove_cvref_t<T>>::ID; };
	}
}