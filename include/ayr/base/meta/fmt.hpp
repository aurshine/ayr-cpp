#ifndef AYR_BASE_META_FMT_HPP
#define AYR_BASE_META_FMT_HPP

#include "CString.hpp"

namespace ayr
{
	template<size_t ArgCount>
	struct CheckedFmtStr
	{
		const char* ptr;
		c_size length;

		template<size_t N>
		consteval CheckedFmtStr(const char(&s)[N]): ptr(s), length(N - 1)
		{
			if (vstr(ptr, length).count(vstr("{}")) != ArgCount)
				fmt_argument_count_mismatch();
		}

		void fmt_argument_count_mismatch() { std::abort(); }
	};

	template<typename... Args>
	constexpr def fmt_to(Buffer& buf, const CheckedFmtStr<sizeof...(Args)>& fmtstr, Args&&... args)
	{
		const CString str = vstr(fmtstr.ptr, fmtstr.length);

		c_size pos = 0;
		if constexpr (sizeof...(Args) > 0)
		{
			constexpr CString PLACEHOLDER = vstr("{}");

			auto append_arg = [&](auto&& arg)
				{
					c_size next = str.index(PLACEHOLDER, pos);

					if (next > pos)
						buf.append_bytes(str.data() + pos, next - pos);

					buf << arg;
					pos = next + PLACEHOLDER.size();
				};

			(append_arg(args), ...);
		}

		if (pos < str.size())
			buf.append_bytes(str.data() + pos, str.size() - pos);
	}

	template<typename... Args>
	constexpr def fmt(const CheckedFmtStr<sizeof...(Args)>& fmtstr, Args&&... args) -> CString
	{
		Buffer buf(fmtstr.length * 2);
		fmt_to(buf, fmtstr, args...);
		return from_buffer(std::move(buf));
	}
}

#endif // AYR_BASE_META_FMT_HPP
