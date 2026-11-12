#ifndef AYR_BASE_META_PRINTER_HPP
#define AYR_BASE_META_PRINTER_HPP

#include "cstr_buffer.hpp"

namespace ayr
{
	class Printer
	{
	public:
		Printer(FILE* file_ptr, const CString& sw = " ", const CString& ew = "\n") : output_file_(file_ptr), sw_(sw), ew_(ew) {}

		template<typename T0, typename... Args>
		void operator()(const T0& object, const Args&... args) const
		{
			Buffer buffer(BUFFER_SIZE);
			write_in_buffer(buffer, object, args...);
			write(buffer.peek(), buffer.readable_size());
		}

		void operator()() const { write(ew_.data(), ew_.size()); }

		// 刷新输出缓冲区
		void flush() const { std::fflush(output_file_); }

		// 设置输出结束符
		void setend(const CString& ew) { ew_ = ew; }

		// 设置输出分隔符
		void setsep(const CString& sw) { sw_ = sw; }

		FILE* file() const { return output_file_; }

		// 输出字符串到文件
		void write(const char* str, size_t len) const { std::fwrite(str, 1, len, output_file_); }

		void write(const CString& str) const { write(str.data(), str.size()); }

		void write(const Buffer& buffer) const { write(buffer.peek(), buffer.readable_size()); }

		static void set_buffer_size(c_size size) { if (size > 0) BUFFER_SIZE = size; }
	protected:
		static c_size BUFFER_SIZE;

		template<typename First, typename... Args>
		void write_in_buffer(Buffer& buffer, First&& first, Args&&... args) const
		{
			buffer << first;
			if constexpr (sizeof...(args) > 0)
				((buffer << sw_ << args), ...);
			buffer << ew_;
		}

		CString ew_; // 结束符

		CString sw_; // 分隔符

		std::FILE* output_file_;
	};

	c_size Printer::BUFFER_SIZE = 128;

	class Color
	{
	public:
		constexpr static const char* CLOSE = "\033[0m";

		constexpr static const char* BLACK = "\033[30m";

		constexpr static const char* RED = "\033[31m";

		constexpr static const char* GREEN = "\033[32m";

		constexpr static const char* YELLOW = "\033[33m";

		constexpr static const char* BLUE = "\033[34m";

		constexpr static const char* PURPLE = "\033[35m";

		constexpr static const char* DEEPGREEN = "\033[36m";

		constexpr static const char* WHITE = "\033[37m";

		constexpr static const char* AQUA = "\033[94m";
	};

	class ColorPrinter : protected Printer
	{
		using self = ColorPrinter;

		using super = Printer;
	public:
		ColorPrinter(FILE* file_ptr, const CString& color = Color::WHITE)
			: Printer(file_ptr), color_(color) {}

		template<typename... Args>
		void operator()(const Args&... args) const
		{
			Buffer buffer(BUFFER_SIZE);
			// write_in_buffer会带分割符和结束符，所以在前后加上颜色控制符
			buffer << color_;
			super::write_in_buffer(buffer, args...);
			buffer << Color::CLOSE;
			super::write(buffer);
		}

		void operator()() const
		{
			Buffer buffer(BUFFER_SIZE);
			buffer << color_ << ew_ << Color::CLOSE;
			super::write(buffer);
		}

		void setcolor(const CString& color) { color_ = color; }
	private:
		CString color_;
	};

	static Printer print{ stdout };

	static ColorPrinter ayr_warner{ stdout, Color::YELLOW };

	static ColorPrinter ayr_error{ stderr, Color::RED };
}
#endif // AYR_BASE_META_PRINTER_HPP
