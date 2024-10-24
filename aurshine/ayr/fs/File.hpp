﻿#ifndef AYR_FS_FILE_HPP
#define AYR_FS_FILE_HPP

#include <cstdio>
#include <atomic>
#include <memory>

#include <ayr/Dynarray.hpp>
#include "Path.hpp"


namespace ayr
{
	namespace fs
	{
		struct BufferMode
		{
			using BufferModeType = int;

			constexpr static BufferModeType NoBuffer = _IONBF;

			constexpr static BufferModeType LineBuffer = _IOLBF;

			constexpr static BufferModeType FullBuffer = _IOFBF;
		};

		class AyrFile : public Object<AyrFile>
		{
		public:
#pragma warning(push)
#pragma warning(disable: 4996)
			template<ConveribleToCstr S1, ConveribleToCstr S2>
			AyrFile(const S1& filename, const S2& mode, c_size buffer_size = DefaultBufferSize, BufferMode::BufferModeType buffer_mode = BufferMode::LineBuffer)
				: file_(std::fopen(filename, mode)), buffer_size_(buffer_size), buffer_(std::make_unique<char[]>(buffer_size))
			{
				std::setvbuf(file_, buffer_.get(), buffer_mode, buffer_size);
				const char* c_filename = static_cast<const char*>(filename);
				if (file_ == nullptr)
				{
					if (!fs::isfile(c_filename))
						FileNotFoundError(std::format("file not found in {}", c_filename));
					else
						RuntimeError(std::format("failed to open file {}", c_filename));
				}
			}
#pragma warning(pop)

			~AyrFile() { std::fclose(file_); }

			template<Printable T>
			void write(const T& data) const { write(cstr(data)); }

			void write(const CString& data) const
			{
				std::fwrite(stdstr(data), 1, data.size(), file_);
			}

			void write(const char* data) const
			{
				std::fwrite(data, 1, std::strlen(data), file_);
			}

			void flush() const { fflush(file_); }

			CString readline() const
			{
				if (fgets(buffer_.get(), buffer_size_, file_))
					RuntimeError("buffer_size is small, failed to readline");

				return CString(buffer_.get());
			}

			Array<CString> readlines() const
			{

			}

			CString read() const
			{

			}

			FILE* file_;

			c_size buffer_size_;

			std::unique_ptr<char[]> buffer_;

			static constexpr c_size DefaultBufferSize = 1024;
		};
	}
}


#endif // AYR_FILE_HPP