#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include <cstdio>
#include <atomic>

#include <law/Printer.hpp>
#include <law/Dynarray.hpp>
#include "Path.hpp"


namespace ayr
{
	namespace fs
	{
		class AyrFile : public Object<AyrFile>
		{
		public:
#pragma warning(push)
#pragma warning(disable: 4996)
			template<ConveribleToCstr S1, ConveribleToCstr S2>
			AyrFile(const S1& filename, const S2& mode) : file(std::fopen(filename, mode)), is_open(false)
			{
				const char* c_filename = static_cast<const char*>(filename);
				if (file == nullptr)
				{
					if (!fs::isfile(filename))
						FileNotFoundError(std::format("file not found in {}", c_filename));
					else
						RuntimeError(std::format("failed to open file {}", c_filename));
				}
			}
#pragma warning(pop)

			~AyrFile() { close(); }

			void close()
			{
				if (is_open)
				{
					std::fclose(file);
					is_open = false;
				}
			}

			// 如果I可以隐式转换为const char*, 则使用fputs, 否则使用for循环逐个写入
			template<typename I>
			void write(const I& data) const
			{
				if constexpr (ConveribleToCstr<I>)
					std::fputs(static_cast<const char*>(data), file);
				else
					for (const char& d : data)
						std::fputc(d, file);
			}

			void flush() const { fflush(file); }

			CString readline() const
			{
				DynArray<char> buffer;
				while (true)
				{
					char c = std::fgetc(file);
					if (c == EOF || c == '\n')
						break;
					buffer.append(c);
				}

				CString ret(buffer.size());
				for (c_size i = 0, length = buffer.size(); i < length; ++i)
					ret[i] = buffer[i];
				return ret;
			}


			std::FILE* file;

			std::atomic<bool> is_open;
		};
	}
}


#endif // AYR_FILE_HPP