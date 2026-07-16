#ifndef AYR_NET_SELECTOR_IORESULT_HPP
#define AYR_NET_SELECTOR_IORESULT_HPP

#include "../../fs/oslib.h"

namespace ayr
{
	namespace net
	{
		// IO事件完成结果
		class IoResult
		{
			using self = IoResult;

#ifdef AYR_WIN
			// Windows下使用DWORD表示错误码
			using Error_t = DWORD;
#elif defined(AYR_LINUX)
			// Linux下使用int表示错误码
			using Error_t = int;
#endif
		public:
			// 错误信息
			CString error;

			/*
			* - READ成功时表示读取到的字节数，bytes == 0 表示对端有序关闭连接。
			* - WRITE成功时表示写出的字节数，bytes == 0 表示本次没有写出数据。
			* - ACCEPT和CONNECT成功时通常为0。
			*/
			int bytes;

			/*
			* - ACCEPT成功时表示新接受的socket。
			* - READ、WRITE、CONNECT成功时表示发起该事件的socket。
			* - 失败时表示与该事件相关的socket，可能需要调用者关闭或丢弃。
			*/
			BaseSocket socket;

			IoResult() : error(), bytes(0), socket(-1) {}

			IoResult(const self& other) : error(other.error.clone()), bytes(other.bytes), socket(other.socket) {}

			self& operator=(const self& other)
			{
				if (this == &other) return *this;
				ayr_destroy(this);
				return *ayr_construct(this, other);
			}

			// 完成时间是否成功
			bool ok() const { return error.empty(); }
		};

		IoResult io_result(CString err, int bytes = 0, BaseSocket socket = -1)
		{
			IoResult res;
			if (err.owner())
				res.error = std::move(err);
			else
				res.error = err.clone();
			res.bytes = bytes;
			res.socket = socket;
			return res;
		}
	}
}
#endif // AYR_NET_SELECTOR_IORESULT_HP