#pragma once
#include <cstring>
#include <string>
#include <algorithm>


namespace ayr
{
	// __str__ ��������ֵ�Ļ�����󳤶�
	constexpr static const int __STR_BUFFER_SIZE__ = 1024;
	// __str__ ��������ֵ�Ļ���
	static char __str_buffer__[__STR_BUFFER_SIZE__]{};

	// �� __str_buffer__ ���и�ֵ
	inline void memcpy__str_buffer__(const char* str, int len=-1)
	{
		if (len < 0) len = std::strlen(str);
		size_t cpy_size = std::min(len, __STR_BUFFER_SIZE__ - 1);

		if (str != __str_buffer__) std::memcpy(__str_buffer__, str, cpy_size);
		__str_buffer__[cpy_size] = '\0';
	}

	inline void memcpy__str_buffer__(const std::string& str)
	{
		memcpy__str_buffer__(str.c_str(), str.size());
	}

	inline void memcpy__str_buffer__(c_size x)
	{
		auto str = std::to_string(x);
		memcpy__str_buffer__(str.c_str(), str.size());
	}

	inline void memcpy__str_buffer__(double x)
	{
		auto str = std::to_string(x);
		memcpy__str_buffer__(str.c_str(), str.size());
	}
}