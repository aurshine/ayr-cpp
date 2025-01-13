#include "../filesystem.hpp"

namespace ayr
{
	class _StartSocket : public Object<_StartSocket>
	{
	public:
#if defined(AYR_WIN)
		_StartSocket()
		{
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				RuntimeError("WSAStartup failed");
		}

		~_StartSocket() { WSACleanup(); }
#endif
	};

	static const _StartSocket __startsocket;
}