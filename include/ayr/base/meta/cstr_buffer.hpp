#ifndef AYR_BASE_META_CSTR_BUFFER_HPP
#define AYR_BASE_META_CSTR_BUFFER_HPP

#include "Buffer.hpp"
#include "CString.hpp"

namespace ayr
{
	// 接管未消费的缓冲区，否则复制剩余字节。
	def from_buffer(Buffer&& buffer) -> CString
	{
		if (buffer.data() == buffer.peek())
		{
			CString result = ostr(buffer.peek(), buffer.readable_size());
			buffer.detach();
			return result;
		}
		return dstr(buffer.peek(), buffer.readable_size());
	}

	// 将任意类型转换为 CString 对象
	template<typename T>
	constexpr def cstr(const T& value)
	{
		if constexpr (hasmethod(T, __str__))
			return value.__str__();
		else
		{
			Buffer buffer;
			buffer << value;
			return from_buffer(std::move(buffer));
		}
	}

	Buffer& operator<<(Buffer& buffer, const CString& value)
	{
		buffer.append_bytes(value.data(), value.size());
		return buffer;
	}
}

#endif // AYR_BASE_META_CSTR_BUFFER_HPP