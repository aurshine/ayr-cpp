#ifndef AYR_BASE_NOCOPY_HPP_
#define AYR_BASE_NOCOPY_HPP_

namespace ayr
{
	struct NoCopy
	{
		NoCopy() = default;

		NoCopy(const NoCopy&) = delete;

		NoCopy& operator=(const NoCopy&) = delete;

		NoCopy(NoCopy&&) = delete;

		NoCopy& operator=(NoCopy&&) = delete;

		~NoCopy() = default;
	};
}
#endif