#ifndef AYR_JSON_TRAIT_HPP
#define AYR_JSON_TRAIT_HPP

#include <law/detail/ayr_concepts.hpp>
#include <law/String.hpp>
#include <law/DynArray.hpp>
#include <law/Dict.hpp>


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

			using JsonStr = Atring;

			using JsonArray = DynArray<Json>;

			using JsonDict = Dict<JsonStr, Json>;

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
		template<JsonTypeStrictConcept T> struct GetJsonTypeIDStrict { constexpr static int8_t ID = -1; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonInt> { constexpr static int8_t ID = 0; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonFloat> { constexpr static int8_t ID = 1; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonBool> { constexpr static int8_t ID = 2; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonStr> { constexpr static int8_t ID = 3; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonArray> { constexpr static int8_t ID = 4; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonDict> { constexpr static int8_t ID = 5; };
		template<> struct GetJsonTypeIDStrict<typename JsonType::JsonNull> { constexpr static int8_t ID = 6; };

		// 可接受const reference volatile 修饰符获得Json类型的枚举值
		template<JsonTypeConcept T>
		struct GetJsonTypeID { constexpr static int8_t ID = GetJsonTypeIDStrict<std::remove_cvref_t<T>>::ID; };
	}
}
#endif