#ifndef AYR_JSON_JSON_BASE_HPP
#define AYR_JSON_JSON_BASE_HPP

#include "../air/Dict.hpp"
#include "../air/DynArray.hpp"

namespace ayr
{
	namespace json
	{
#define JSON_TYPE_INVALID_ERROR RuntimeError("Invalid Json Type")

		class Json;

		using JsonNull = _None;

		using JsonInt = long long;

		using JsonFloat = double;

		using JsonBool = bool;

		using JsonStr = Atring;

		using JsonArray = DynArray<Json>;

		using JsonDict = Dict<JsonStr, Json>;

		template<typename T> struct GetJsonTypeID { constexpr static int8_t ID = -1; };
		template<> struct GetJsonTypeID<JsonNull> { constexpr static int8_t ID = 0; };
		template<> struct GetJsonTypeID<JsonInt> { constexpr static int8_t ID = 1; };
		template<> struct GetJsonTypeID<JsonFloat> { constexpr static int8_t ID = 2; };
		template<> struct GetJsonTypeID<JsonBool> { constexpr static int8_t ID = 3; };
		template<> struct GetJsonTypeID<JsonStr> { constexpr static int8_t ID = 4; };
		template<> struct GetJsonTypeID<JsonArray> { constexpr static int8_t ID = 5; };
		template<> struct GetJsonTypeID<JsonDict> { constexpr static int8_t ID = 6; };

		template<typename T>
		concept JsonTypeConcept = issame<T, JsonNull, JsonInt, JsonFloat, JsonBool, JsonStr, JsonArray, JsonDict>;

		constexpr int8_t INVALID_JSON_TYPE_ID = -1;

		template<typename T>
		def json_invalid_error() { RuntimeError(std::format("Invalid json type: {}", dtype(T))); }
	}
}
#endif // AYR_JSON_JSON_BASE_HPP