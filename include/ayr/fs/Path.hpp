#ifndef AYR_FS_PATH_HPP
#define AYR_FS_PATH_HPP

#include <cstdlib>
#include <queue>

#include "oslib.h"
#include "../air/DynArray.hpp"
#include "../coro/Generator.hpp"

namespace ayr
{
	namespace fs
	{
#ifdef AYR_WIN
		constexpr char PATH_SEP = '\\';
#else
		constexpr char PATH_SEP = '/';
#endif

		/**
		 * @brief 判断字符是否是当前平台支持的路径分隔符。
		 * 
		 * @param c 待判断的字符。
		 * 
		 * @return Windows 下遇到 `\` 或 `/` 返回 true；POSIX 下仅 `/` 返回 true。
		 */
		def __is_path_sep(char c)
		{
#ifdef AYR_WIN
			return c == '\\' || c == '/';
#else
			return c == '/';
#endif
		}

		// 获取最后一个路径分隔符的下标
		def __last_sep_index(const CString& path) -> c_size
		{
			c_size last_sep_pos = -1;
			for (c_size i = path.size() - 1; i >= 0; --i)
				if (__is_path_sep(path[i]))
					return i;
			return -1;
		}

		/**
		 * @brief 使用最近一次系统错误构造并抛出包含操作名称和路径的 SystemError。
		 * 
		 * @param operation 失败操作的文字说明。
		 * 
		 * @param path 发生错误的路径字节串。
		 * 
		 * @throws AyrError 始终抛出，错误消息包含 get_system_error_msg() 的结果。
		 */
		def raise_error_msg_for_path(const CString& operation, const CString& path)
		{
			CString error_msg = get_system_error_msg();
			SystemError(ayr::format("{} {}: {}", operation, path, error_msg));
		}

		/**
		 * @brief 判断最近一次系统错误是否表示路径不存在或中间目录不存在。
		 * 
		 * @return 属于“路径不存在”错误时返回 true，否则返回 false。
		 */
		def is_missing_error()
		{
#ifdef AYR_WIN
			DWORD error = GetLastError();
			return error == ERROR_FILE_NOT_FOUND ||
				error == ERROR_PATH_NOT_FOUND ||
				error == ERROR_INVALID_NAME;
#else
			return errno == ENOENT || errno == ENOTDIR;
#endif
		}

#ifdef AYR_WIN
		/**
		 * @brief 查询 Windows 路径属性，并区分路径不存在与其他系统错误。
		 * 
		 * @param path 路径字节串。
		 * 
		 * @param attributes 成功时写入 Win32 文件属性。
		 * 
		 * @return 查询成功返回 true；路径不存在返回 false。
		 * 
		 * @throws AyrError 查询因权限、I/O 等非“不存在”原因失败时抛出。
		 */
		def get_path_attributes(const CString& path, DWORD& attributes)
		{
			attributes = GetFileAttributesA(path.c_str());
			if (attributes != INVALID_FILE_ATTRIBUTES)
				return true;
			if (is_missing_error())
				return false;
			raise_error_msg_for_path("Failed to query path", path);
			return false;
		}
#else
		/**
		 * @brief 查询 POSIX 路径状态，并可选择是否跟随符号链接。
		 * 
		 * @param path 路径。
		 * 
		 * @param st 成功时写入 stat 结果。
		 * 
		 * @param follow_links 为 true 时使用 stat，为 false 时使用 lstat。
		 * 
		 * @return 查询成功返回 true；路径不存在返回 false。
		 * 
		 * @throws AyrError 查询因权限、I/O 等非“不存在”原因失败时抛出。
		 */
		def get_path_stat(const CString& path, struct stat& st, bool follow_links)
		{
			int state = ifelse(follow_links, 
				::stat(path.c_str(), &st), 
				::lstat(path.c_str(), &st)
			);
			if (state == 0)
				return true;
			if (is_missing_error())
				return false;
			raise_error_msg_for_path("Failed to query path", path);
			return false;
		}
#endif

		/**
		 * @brief 判断路径是否为绝对路径。
		 * 
		 * @param path 待判断路径。
		 * 
		 * @return Windows 下驱动器绝对路径或 UNC 路径返回 true；POSIX 下以 `/` 开头返回 true。
		 */
		def isabs(const CString& path)
		{
			if (path.empty()) return false;
#ifdef AYR_WIN
			// 驱动器绝对路径（C:\path）或 UNC 路径（\\server\share）。
			return (
				(path.size() >= 3 && std::isalpha(path[0]) && path[1] == ':' && __is_path_sep(path[2]))) ||
				(path.size() >= 2 && __is_path_sep(path[0]) && __is_path_sep(path[1]));
#else
			return path[0] == '/';
#endif
		}

		/**
		 * @brief 使用当前平台分隔符拼接两个路径。
		 * 
		 * @param path1 前半部分路径。
		 * 
		 * @param path2 后半部分路径；若其自身为绝对路径，则直接返回它的副本。
		 * 
		 * @return 拼接后的拥有内存的 CString。
		 */
		def join(const CString& path1, const CString& path2)
		{
			if (path1.empty())
				return path2;
			if (path2.empty())
				return path1;
			if (isabs(path2))
				return path2;

			c_size len1 = path1.size();
			c_size len2 = path2.size();
			while (len1 > 0 && __is_path_sep(path1[len1 - 1]))
				--len1;

			c_size i = 0;
			while (i < len2 && __is_path_sep(path2[i]))
				++i;

			return CString::cjoin(arr(path1.slice(0, len1), cstr(PATH_SEP), path2.slice(i)));
		}

		/**
		 * @brief 判断路径或符号链接本身是否存在。
		 * 
		 * @param path 待查询路径。
		 * 
		 * @return 存在返回 true，不存在返回 false。
		 * 
		 * @throws AyrError 无法查询路径且原因不是“不存在”时抛出。
		 */
		def exists(const CString& path)
		{
#ifdef AYR_WIN
			DWORD attributes;
			return get_path_attributes(path, attributes);
#else
			struct stat st;
			return get_path_stat(path, st, false);
#endif
		}

		/**
		 * @brief 判断路径是否指向普通文件。
		 * 
		 * @param path 待查询路径。
		 * 
		 * @return 路径指向普通文件时返回 true；不存在或类型不符时返回 false。
		 * 
		 * @note POSIX 下会跟随符号链接。
		 * 
		 * @throws AyrError 查询发生权限或其他系统错误时抛出。
		 */
		def isfile(const CString& path)
		{
#ifdef AYR_WIN
			DWORD attributes;
			return get_path_attributes(path, attributes) && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
			struct stat st;
			return get_path_stat(path, st, true) && S_ISREG(st.st_mode);
#endif
		}

		/**
		 * @brief 判断路径是否指向目录。
		 * 
		 * @param path 待查询路径。
		 * 
		 * @return 路径指向目录时返回 true；不存在或类型不符时返回 false。
		 * 
		 * @note POSIX 下会跟随符号链接。
		 * 
		 * @throws AyrError 查询发生权限或其他系统错误时抛出。
		 */
		def isdir(const CString& path)
		{
#ifdef AYR_WIN
			DWORD attributes;
			return get_path_attributes(path, attributes) && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
			struct stat st;
			return get_path_stat(path, st, true) && S_ISDIR(st.st_mode);
#endif
		}

		/**
		 * @brief 获取进程当前工作目录。
		 * 
		 * @return 当前平台文件系统 API 返回的绝对目录路径字节串。
		 * 
		 * @throws AyrError 系统无法取得当前目录时抛出。
		 */
		def getcwd()
		{
#ifdef AYR_WIN
			DWORD required = GetCurrentDirectoryA(0, nullptr);
			if (required == 0)
				SystemError(ayr::format("getcwd failed: {}", get_system_error_msg()));
			Buffer cwd_path(required);
			DWORD len = GetCurrentDirectoryA(required, cwd_path.write_ptr());
			if (len == 0 || len >= required)
				SystemError(ayr::format("getcwd failed: {}", get_system_error_msg()));
#else
			Buffer cwd_path(256);
			while (::getcwd(cwd_path.write_ptr(), cwd_path.capacity()) == nullptr)
			{
				if (errno != ERANGE)
					SystemError(ayr::format("getcwd failed: {}", get_system_error_msg()));
				cwd_path.adjust_util(cwd_path.capacity() + 10);
			}
			int len = ayr::strlen(cwd_path.peek());
#endif		
			cwd_path.written(len);
			return from_buffer(std::move(cwd_path));
		}

		/**
		 * @brief 将路径转换为绝对路径。
		 * 
		 * @param path 相对或绝对路径。
		 * 
		 * @return 绝对路径字节串。
		 * 
		 * @note POSIX 下不要求目标存在；Windows 下由 GetFullPathNameA 解析。
		 * 
		 * @throws AyrError 路径解析或获取当前目录失败时抛出。
		 */
		def abspath(const CString& path)
		{
#ifdef AYR_WIN
			DWORD required = GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
			if (required == 0)
				raise_error_msg_for_path("Failed to get absolute path for", path);
			Buffer abs_path(required);
			DWORD len = GetFullPathNameA(path.c_str(), required, abs_path.write_ptr(), nullptr);
			if (len == 0 || len >= required)
				raise_error_msg_for_path("Failed to get absolute path for", path);
			
			abs_path.written(len);
			return from_buffer(std::move(abs_path));
#else
			if (isabs(path))
				return path;
			return join(getcwd(), path);
#endif 
		}

		/**
		 * @brief 惰性枚举目录中的直接子项。
		 * 
		 * @param path 待枚举目录。
		 * 
		 * @return 依次产生子项名称的生成器，不包含 `.` 和 `..`，也不包含父目录前缀。
		 * 
		 * @throws AyrError 打开目录或枚举过程发生系统错误时抛出。
		 */
		def listdir(const CString& path) -> coro::Generator<CString>
		{
			// 协程可能比调用表达式活得更久，不能在帧中保留调用者字符串的浅引用。
			CString owned_path = path;
#ifdef AYR_WIN
			WIN32_FIND_DATAA find_data;

			CString pattern = join(owned_path, "*");
			HANDLE handle = FindFirstFileA(pattern.c_str(), &find_data);

			if (handle == INVALID_HANDLE_VALUE)
				raise_error_msg_for_path("Failed to list directory", owned_path);

			exitask([&handle] { FindClose(handle); });

			do
			{
				CString file_name = dstr(find_data.cFileName);
				if (file_name == "." || file_name == "..")
					continue;

				co_yield std::move(file_name);
			} while (FindNextFileA(handle, &find_data));

			if (GetLastError() != ERROR_NO_MORE_FILES)
				raise_error_msg_for_path("Failed while listing directory", owned_path);
#else
			DIR* dir = opendir(owned_path.c_str());

			if (dir == nullptr)
				raise_error_msg_for_path("Failed to list directory", owned_path);

			exitask([&dir] { closedir(dir); });

			while (true)
			{
				errno = 0;
				struct dirent* entry = readdir(dir);
				if (entry == nullptr)
				{
					if (errno != 0)
						raise_error_msg_for_path("Failed while listing directory", owned_path);
					break;
				}
				CString d_name = dstr(entry->d_name);
				if (d_name == "." || d_name == "..")
					continue;
				co_yield std::move(d_name);
			}
#endif
		}

		/**
		 * @brief 以广度优先顺序递归遍历目录树。
		 * 
		 * @param path 遍历起点。
		 * 
		 * @return 依次产生 `(当前根目录, 子目录名称数组, 非目录项名称数组)` `(root, dirs, files)`。
		 * 
		 * @note 不进入 POSIX 符号链接或 Windows reparse point，避免链接环和越界遍历。
		 * 
		 * @throws AyrError 查询或枚举目录失败时抛出。
		 */
		def walk(const CString& path) -> coro::Generator<std::tuple<CString, DynArray<CString>, DynArray<CString>>>
		{
			std::queue<CString> root_dirs;
			root_dirs.push(path);

			while (!root_dirs.empty())
			{
				CString root = std::move(root_dirs.front());
				root_dirs.pop();

				DynArray<CString> dirs, files;
				for (auto& sub_path : listdir(root))
				{
					CString child_path = join(root, sub_path);
#ifdef AYR_WIN
					DWORD attributes;
					if (!get_path_attributes(child_path, attributes))
						continue;
					bool traversable_directory =
						(attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
						(attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
#else
					struct stat st;
					if (!get_path_stat(child_path, st, false))
						continue;
					bool traversable_directory = S_ISDIR(st.st_mode);
#endif
					if (!traversable_directory)
						files.append(sub_path);
					else
					{
						dirs.append(sub_path);
						root_dirs.push(std::move(child_path));
					}
				}

				co_yield std::make_tuple(std::move(root), std::move(dirs), std::move(files));
			}
			co_return coro::finish;
		}

		/**
		 * @brief 创建单层目录。
		 * 
		 * @param path 要创建的目录路径；不会自动创建缺失的父目录。
		 * 
		 * @param exist_ok 为 true 且路径已是目录时静默成功；同名普通文件仍报错。
		 * 
		 * @throws AyrError 创建失败或目录已存在且 exist_ok 为 false 时抛出。
		 */
		def mkdir(const CString& path, bool exist_ok = false)
		{
#ifdef AYR_WIN
			if (CreateDirectoryA(path.c_str(), nullptr))
				return;
			DWORD error = GetLastError();
			CString error_msg = win_error2str(error);
			if (error == ERROR_ALREADY_EXISTS && exist_ok)
			{
				DWORD attributes;
				if (get_path_attributes(path, attributes) &&
					(attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					return;
			}
#else
			if (::mkdir(path.c_str(), 0755) == 0)
				return;
			
			int error = errno;
			CString error_msg = c_error2str(error);
			if (error == EEXIST && exist_ok)
			{
				struct stat st;
				if (get_path_stat(path, st, true) && S_ISDIR(st.st_mode))
					return;
			}
#endif
			SystemError(ayr::format("Failed to create directory {}: {}", path, error_msg));
		}

		/**
		 * @brief 删除文件、链接或递归删除目录树。
		 * 
		 * @param path 待删除路径；路径不存在时不执行操作。
		 * 
		 * @note 目录符号链接和 Windows junction 只删除链接本身，不遍历目标目录。
		 * 
		 * @throws AyrError 查询、枚举或删除任一步骤失败时抛出。
		 */
		def remove(const CString& path)
		{
#ifdef AYR_WIN
			DWORD attributes;
			if (!get_path_attributes(path, attributes))
				return;
			bool is_directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			bool is_reparse_point = (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
			if (!is_directory)
			{
				if (!DeleteFileA(path.c_str()))
					raise_error_msg_for_path("Failed to delete file", path);
				return;
			}
			if (!is_reparse_point)
				for (auto& sub_path : listdir(path))
					remove(join(path, sub_path));

			if (!RemoveDirectoryA(path.c_str()))
				raise_error_msg_for_path(
					ifelse(is_reparse_point, "Failed to delete directory link", "Failed to delete directory"),
					path
				);
#else
			struct stat st;
			if (!get_path_stat(path, st, false))
				return;

			if (S_ISDIR(st.st_mode))
			{
				for (auto& sub_path : listdir(path))
					remove(join(path, sub_path));

				if (::rmdir(path.c_str()) != 0)
					raise_error_msg_for_path("Failed to delete directory", path);
			}
			else if (::unlink(path.c_str()) != 0)
				raise_error_msg_for_path("Failed to delete file or link", path);
#endif
		}

		/**
		 * @brief 计算文件大小或递归汇总目录内容大小。
		 * @param path 文件或目录路径。
		 * @return 字节数；Windows reparse point 返回 0，且不会进入其目标。
		 * @throws AyrError 路径不存在、查询失败、枚举失败或累加溢出时抛出。
		 */
		def filesize(const CString& path) -> uint64_t
		{
#ifdef AYR_WIN
			DWORD attributes;
			if (!get_path_attributes(path, attributes))
				raise_error_msg_for_path("Cannot get size of missing path", path);

			bool is_directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			bool is_reparse_point = (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

			if (is_reparse_point)
				return 0;

			if (!is_directory)
			{
				WIN32_FILE_ATTRIBUTE_DATA attr{};
				if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &attr))
					raise_error_msg_for_path("Failed to get file size for", path);
				return (static_cast<uint64_t>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;
			}
#else
			struct stat st;
			if (!get_path_stat(path, st, false))
				raise_error_msg_for_path("Cannot get size of missing path", path);

			if (!S_ISDIR(st.st_mode))
				return std::max<uint64_t>(0, st.st_size);
#endif

			uint64_t size_ = 0;
			for (auto&& sub_path : listdir(path))
			{
				uint64_t child_size = filesize(join(path, sub_path));
				if (UINT64_MAX - size_ < child_size)
					SystemError(ayr::format("Directory size overflow while reading {}", path));
				size_ += child_size;
			}
			return size_;
		}

		/**
		 * @brief 提取路径最后一个分隔符之后的基本名。
		 * 
		 * @param path 输入路径。
		 * 
		 * @return 基本名；路径以分隔符结尾时返回空字符串。
		 */
		def basename(const CString& path)
		{
			c_size last_sep_pos = -1;
			for (c_size i = path.size() - 1; i >= 0; --i)
				if (__is_path_sep(path[i]))
				{
					last_sep_pos = i;
					break;
				}

			return path.slice(last_sep_pos + 1);
		}

		/**
		 * @brief 提取路径中基本名之前的目录部分。
		 * 
		 * @param path 输入路径。
		 * 
		 * @return 目录部分；没有分隔符时返回空字符串，根目录返回平台根分隔符。
		 */
		def dirname(const CString& path)
		{
			c_size last_sep_pos = __last_sep_index(path);

			if (last_sep_pos == -1)
				return vstr("");
			else if (last_sep_pos == 0)
				return cstr(PATH_SEP);

			return path.slice(0, last_sep_pos);
		}

		/**
		 * @brief 将路径拆分为目录部分和基本名。
		 * 
		 * @param path 输入路径。
		 * 
		 * @return `(dirname(path), basename(path))`。
		 */
		def split(const CString& path)
		{
			c_size last_sep_pos = __last_sep_index(path);
			
			if (last_sep_pos == -1)
				return std::make_pair(vstr(""), path.slice(last_sep_pos + 1));
			else if (last_sep_pos == 0)
				return std::make_pair(cstr(PATH_SEP), path.slice(last_sep_pos + 1));
			else
				return std::make_pair(path.slice(0, last_sep_pos), path.slice(last_sep_pos + 1));
		}

		/**
		 * @brief 将路径拆分为无扩展名部分和扩展名。
		 * @param path 输入路径。
		 * @return `(主体, 扩展名)`；扩展名包含前导点。
		 * @note 不把目录名中的点或文件名开头的单个点视为扩展名。
		 */
		def splitext(const CString& path)
		{
			c_size name_start = __last_sep_index(path) + 1;

			c_size last_dot_pos = -1;
			for (c_size i = path.size() - 1; i > name_start; --i)
				if (path[i] == '.')
				{
					last_dot_pos = i;
					break;
				}

			if (last_dot_pos == -1) last_dot_pos = path.size();

			return std::make_pair(
				path.slice(0, last_dot_pos),
				path.slice(last_dot_pos)
			);
		}
	}
}
#endif // AYR_FS_PATH_HPP
