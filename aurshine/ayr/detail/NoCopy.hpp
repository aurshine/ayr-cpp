#ifndef AYR_DETAIL_NOCOPY_HPP_
#define AYR_DETAIL_NOCOPY_HPP_

struct NoCopy
{
	NoCopy() = default;

	NoCopy(const NoCopy&) = delete;

	NoCopy& operator=(const NoCopy&) = delete;

	NoCopy(NoCopy&&) = delete;

	NoCopy& operator=(NoCopy&&) = delete;

	~NoCopy() = default;
};
#endif