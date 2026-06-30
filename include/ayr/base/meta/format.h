#ifndef AYR_BASE_META_FORMAT_H
#define AYR_BASE_META_FORMAT_H

#ifndef AYR_USE_FMT
#define AYR_USE_FMT 0
#endif

#if AYR_USE_FMT
#include <fmt/format.h>
#else
#include <format>
#endif

namespace ayr
{
#if AYR_USE_FMT
	using fmt::format;
#else
	using std::format;
#endif
}

#endif // AYR_BASE_META_FORMAT_H
