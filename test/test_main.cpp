#include <ayr/base/itertools.hpp>
#include <ayr/base/raise_error.hpp>

using namespace ayr;
template <typename Range>
class enumerate_view {
public:
	explicit enumerate_view(Range base) : base_(std::move(base)) {}
private:
	Range base_;
};
#include <vector>

int main()
{
	for (auto& i : range(10))
		print(i);
	for (auto [i, x] : enumerate(range(10)))
		print(i, x);

	for (auto [x, y] : zip(range(2, 5), range(3, 6)))
		print(x, y);

	return 0;
}