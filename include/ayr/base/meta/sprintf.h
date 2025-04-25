#ifndef AYR_BASE_META_SPRINTF_H
#define AYR_BASE_META_SPRINTF_H

#include <cstdio>

#include "ayr.h"

namespace ayr
{
	def sprintf_int(char* buffer, c_size size, c_size value)
	{
#ifdef _MSC_VER
		sprintf_s(buffer, size, "%lld", value);
#else
		std::sprintf(buffer, "%lld", value);
#endif
	}

	def sprintf_float(char* buffer, c_size size, double value)
	{
#ifdef _MSC_VER
		sprintf_s(buffer, size, "%lf", value);
#else
		std::sprintf(buffer, "%lf", value);
#endif
	}

	def sprintf_pointer(char* buffer, c_size size, const void* value)
	{
#ifdef _MSC_VER
		sprintf_s(buffer, size, "%p", value);
#else
		std::sprintf(buffer, "%p", value);
#endif
	}
}
#endif // AYR_BASE_META_SPRINTF_H