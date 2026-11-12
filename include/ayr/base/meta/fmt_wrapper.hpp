#ifndef AYR_BASE_META_FMT_WRAPPER_HPP
#define AYR_BASE_META_FMT_WRAPPER_HPP

#include <cstdint>
#include <limits>

#include "cstr_buffer.hpp"

namespace ayr
{
	template<std::integral Integer>
	class BaseInt
	{
		Integer value;

		int base;
	public:
		BaseInt(Integer value, int base = 10) : value(value), base(base) {}

		void __repr__(Buffer& buffer) const { buffer.append_number(value, base); }
	};

	template<std::floating_point Float>
	class Fixed
	{
		Float value;

		int precision;
	public:
		Fixed(Float value, int precision = -1) : value(value), precision(precision) {}

		void __repr__(Buffer& buffer) const { buffer.append_number(value, precision); }
	};

	
	class Pad
	{
	public:
		enum class Align
		{
			Left,
			Right,
			Center
		};

		Pad(CString value, c_size width, Align align = Align::Right, char fill = ' ')
			: value(value), width(width), align(align), fill(fill) {}

		void __repr__(Buffer& buffer) const
		{
			c_size padding = std::max<c_size>(0, width - value.size());
			c_size left_padding = 0, right_padding = 0;

			if (align == Align::Left)
				right_padding = padding;
			else if (align == Align::Right)
				left_padding = padding;
			else if (align == Align::Center)
			{
				left_padding = padding / 2;
				right_padding = padding - left_padding;
			}

			if (left_padding > 0)
				buffer.append_bytes(&fill, 1, left_padding);
			buffer << value;
			if (right_padding > 0)
				buffer.append_bytes(&fill, 1, right_padding);
		}
	private:
		CString value;

		c_size width;

		Align align;

		char fill;
	};

	template<std::integral Integer>
	def baseint(Integer value, int base = 10) { return BaseInt<Integer>(value, base); }

	template<std::floating_point Float>
	def fixed(Float value, int precision = -1) { return Fixed<Float>(value, precision); }

	template<typename T>
	def pad(const T& value, c_size width, Pad::Align align, char fill = ' ') { return Pad(cstr(value), width, align, fill); }

	template<typename T>
	def left_pad(const T& value, c_size width, char fill = ' ') { return Pad(cstr(value), width, Pad::Align::Left, fill); }

	template<typename T>
	def right_pad(const T& value, c_size width, char fill = ' ') { return Pad(cstr(value), width, Pad::Align::Right, fill); }

	template<typename T>
	def center_pad(const T& value, c_size width, char fill = ' ') { return Pad(cstr(value), width, Pad::Align::Center, fill); }
}
#endif // AYR_BASE_META_FMT_WRAPPER_HPP
