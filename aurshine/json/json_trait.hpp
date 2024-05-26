#pragma once
#include <concepts>
#include <type_traits>

namespace ayr
{
	namespace json
	{
		class Json;
		class Null {};

		using JsonInt = long long;
		using JsonDouble = double;
		using JsonBool = bool;
		using JsonStr = std::string;
		using JsonArray = std::vector<Json>;
		using JsonDict = std::map<JsonStr, Json>;
		using JsonNull = Null;

		template<typename T>
		concept JsonTypeStrictConcept = std::is_same_v<T, JsonInt>
			|| std::is_same_v<T, JsonDouble>
			|| std::is_same_v<T, JsonBool>
			|| std::is_same_v<T, JsonStr>
			|| std::is_same_v<T, JsonArray>
			|| std::is_same_v<T, JsonDict>
			|| std::is_same_v<T, JsonNull>;

		template<typename T>
		concept JsonTypeConcept = JsonTypeStrictConcept<std::remove_cvref_t<T>>;

		// 严格类型获得Json类型的枚举值
		template<JsonTypeStrictConcept T> struct GetJsonTypeEnumStrict { constexpr static signed ID = -1; };
		template<> struct GetJsonTypeEnumStrict<JsonInt> { constexpr static signed ID = 0; };
		template<> struct GetJsonTypeEnumStrict<JsonDouble> { constexpr static signed ID = 1; };
		template<> struct GetJsonTypeEnumStrict<JsonBool> { constexpr static signed ID = 2; };
		template<> struct GetJsonTypeEnumStrict<JsonStr> { constexpr static signed ID = 3; };
		template<> struct GetJsonTypeEnumStrict<JsonArray> { constexpr static signed ID = 4; };
		template<> struct GetJsonTypeEnumStrict<JsonDict> { constexpr static signed ID = 5; };
		template<> struct GetJsonTypeEnumStrict<JsonNull> { constexpr static signed ID = 6; };

		// 可接受const reference volatile 修饰符获得Json类型的枚举值
		template<JsonTypeConcept T>
		struct GetJsonTypeEnum { constexpr static signed ID = GetJsonTypeEnumStrict<std::remove_cvref_t<T>>::ID; };
	}
}