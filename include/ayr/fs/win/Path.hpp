#ifndef AYR_FS_WIN_PATH_HPP
#define AYR_FS_WIN_PATH_HPP

#include "winlib.hpp"
#include "../../Atring.hpp"
#include "../../DynArray.hpp"


namespace ayr
{
	namespace fs
	{
		constexpr char PATH_SEP = '\\';

		enum class PathStatus
		{
			UNKNOWN,
			NOT_EXISTS,
			IS_FILE,
			IS_DIR
		};

		class Path : public Object<Path>
		{
			using self = Path;

			using super = Object<Path>;

			PathStatus status_;

			CString path_;
		public:
			Path(const char* path) : path_(path), status_(PathStatus::UNKNOWN) { identify(); }

			Path(CString path) : path_(std::move(path)), status_(PathStatus::UNKNOWN) { identify(); }

			Path(const Atring& path) : path_(cstr(path)), status_(PathStatus::UNKNOWN) { identify(); }

			Path(self&& other) : path_(other.path_), status_(other.status_)
			{
				other.path_ = "";
				other.status_ = PathStatus::UNKNOWN;
			}

			self& operator= (self&& other)
			{
				path_ = other.path_;
				status_ = other.status_;
				other.path_ = "";
				other.status_ = PathStatus::UNKNOWN;
				return *this;
			}

			// 是否存在路径
			bool exists() const { return status_ != PathStatus::NOT_EXISTS; }

			// 是否是文件
			bool isfile() const { return status_ == PathStatus::IS_FILE; }

			// 是否是文件夹
			bool isdir() const { return status_ == PathStatus::IS_DIR; }

			// 是否是绝对路径
			bool isabs() const
			{
				return (path_[0] && path_[1] == ':' && is_sep(path_[2])) || (path_[0] == '\\' && path_[1] == '\\');
			}

			// 获得绝对路径
			CString abspath() const
			{
				char full_path[MAX_PATH];
				DWORD result = GetFullPathNameA(path_, MAX_PATH, full_path, nullptr);

				if (result == 0 || result > MAX_PATH)
					RuntimeError(std::format("GetFullPathNameA failed for path: {}", path_));

				return CString(full_path, result);
			}

			// 用当前路径创建一个文件夹
			void mkdir(bool exist_ok = false)
			{
				BOOL state = CreateDirectoryA(path_, nullptr);

				switch (state)
				{
				case ERROR_ALREADY_EXISTS:
					if (!exist_ok)
						RuntimeError(std::format("Directory already exists: {}", path_));
					break;
				case ERROR_PATH_NOT_FOUND:
					FileNotFoundError(std::format("Path not found: {}", path_));
					break;
				}

				status_ = PathStatus::IS_DIR;
			}

			// 删除当前路径的文件或文件夹
			void remove()
			{
				if (!exists())
					FileNotFoundError(std::format("File or Directory not found in {}", path_));

				if (isfile() && !DeleteFileA(path_))
					SystemError("Failed to remove file");
				else if (isdir() && !RemoveDirectoryA(path_))
					SystemError("Failed to remove directory");
				status_ = PathStatus::NOT_EXISTS;
			}

			// 拼接两个路径
			static CString join(const char* path1, const char* path2)
			{
				c_size len1 = strlen(path1);
				c_size len2 = strlen(path2);
				if (is_sep(path1[len1 - 1]))
					--len1;
				if (is_sep(path2[0]))
					++path2, --len2;

				CString result{ len1 + len2 + 1 };
				std::memcpy(result.data(), path1, len1);
				std::memcpy(result.data() + len1, &PATH_SEP, 1);
				std::memcpy(result.data() + len1 + 1, path2, len2);
				return result;
			}

			// 列出当前路径下的所有文件和文件夹
			static Array<CString> listdir(const char* path)
			{
				WIN32_FIND_DATAA find_data;
				HANDLE handle = FindFirstFileA(join(path, "*"), &find_data);

				if (handle == INVALID_HANDLE_VALUE)
					SystemError(std::format("Failed to list directory: {}", path));

				DynArray<CString> results;
				do
				{
					if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
						continue;
					results.append(find_data.cFileName);
				} while (FindNextFileA(handle, &find_data));

				FindClose(handle);
				return results.to_array();
			}

			// 获得路径的基本名
			static CString basename(const char* path)
			{
				c_size last_sep_pos = -1, pos = 0;
				while (path[pos])
				{
					if (is_sep(path[pos])) last_sep_pos = pos;
					++pos;
				}

				return CString(path + last_sep_pos + 1, pos - last_sep_pos - 1);
			}

			// 获得路径的目录
			static CString dirname(const char* path)
			{
				c_size last_sep_pos = 0, pos = 0;
				while (is_sep(path[pos])) ++pos, ++last_sep_pos;
				while (path[pos])
				{
					if (is_sep(path[pos])) last_sep_pos = pos;
					++pos;
				}

				return CString(path, last_sep_pos);
			}

			// 获得路径的目录和文件名
			static std::pair<CString, CString> split(const char* path)
			{
				return std::make_pair(dirname(path), basename(path));
			}

			// 获得文件扩展名
			static std::pair<CString, CString> splitext(const char* path)
			{
				c_size last_dot_pos = -1, pos = 0;
				while (path[pos] == '.') ++pos;
				while (path[pos])
				{
					if (path[pos] == '.') last_dot_pos = pos;
					++pos;
				}

				if (last_dot_pos == -1) last_dot_pos = pos;
				return std::make_pair(CString(path, last_dot_pos), CString(path + last_dot_pos, pos - last_dot_pos));
			}

			CString __str__() const { return path_; }

			bool __equals__(const self& other) const { return path_ == other.path_; }

			cmp_t __cmp__(const self& other) const { return path_.__cmp__(other.path_); }

			hash_t __hash__() const { return path_.__hash__(); }
		private:
			static bool is_sep(char c) { return c == '\\' || c == '/'; }

			void identify()
			{
				DWORD attributes = GetFileAttributesA(path_);

				if (attributes == INVALID_FILE_ATTRIBUTES)
					status_ = PathStatus::NOT_EXISTS;
				else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					status_ = PathStatus::IS_DIR;
				else
					status_ = PathStatus::IS_FILE;
			}
		};
	}
}

#endif 