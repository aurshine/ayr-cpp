#include <vector>

#include <ayr/air/Chain.hpp>

#include <ayr/base/utest.hpp>

using namespace ayr;

int main()
{
	Chain<int> chain;
	Chain<int>::Node_t* second = nullptr;
	UTEST_SCOPE("测试空链表追加元素后的 size/front/back/at。")
	{
		UTEST_EXPECT_EQ(chain.size(), 0);
		auto* first = chain.append(1);
		second = chain.append(2);
		auto* third = chain.append(3);
		UTEST_EXPECT_EQ(chain.size(), 3);
		UTEST_EXPECT_EQ(chain.front(), 1);
		UTEST_EXPECT_EQ(chain.back(), 3);
		UTEST_EXPECT_EQ(chain.at(1), 2);
		UTEST_EXPECT(first->next() == second);
		UTEST_EXPECT(third->prev() == second);
	};

	UTEST_SCOPE("测试正向迭代、反向迭代和迭代器距离。")
	{
		int forward_sum = 0;
		for (int value : chain)
			forward_sum += value;
		UTEST_EXPECT_EQ(forward_sum, 6);
		int reverse_sum = 0;
		for (auto it = chain.rbegin(); it != chain.rend(); --it)
			reverse_sum += *it;
		UTEST_EXPECT_EQ(reverse_sum, 6);
		UTEST_EXPECT_EQ(chain.end() - chain.begin(), 3);
	};

	UTEST_SCOPE("测试删除中间节点、头部区间和尾部区间。")
	{
		chain.pop(second);
		UTEST_EXPECT_EQ(chain.size(), 2);
		UTEST_EXPECT_EQ(chain.front(), 1);
		UTEST_EXPECT_EQ(chain.back(), 3);
		chain.pop_front();
		UTEST_EXPECT_EQ(chain.size(), 1);
		UTEST_EXPECT_EQ(chain.front(), 3);
		chain.append(4);
		chain.append(5);
		chain.pop_back(2);
		UTEST_EXPECT_EQ(chain.size(), 1);
		UTEST_EXPECT_EQ(chain.back(), 3);
	};

	Chain<int> from_vector;
	UTEST_SCOPE("测试拷贝、移动和由可迭代对象构造。")
	{
		Chain<int> copied(chain);
		UTEST_EXPECT_EQ(copied, chain);
		Chain<int> moved(std::move(copied));
		UTEST_EXPECT_EQ(moved.size(), 1);
		std::vector<int> values{ 7, 8, 9 };
		from_vector = ayr::chain<int>(values);
		UTEST_EXPECT_EQ(from_vector.size(), 3);
		UTEST_EXPECT_EQ(from_vector.front(), 7);
		UTEST_EXPECT_EQ(from_vector.back(), 9);
	};

	UTEST_SCOPE("测试 clear 后链表可继续追加。")
	{
		from_vector.clear();
		UTEST_EXPECT_EQ(from_vector.size(), 0);
		from_vector.append(10);
		UTEST_EXPECT_EQ(from_vector.front(), 10);
	};

	return UTEST_COMPLETE();
}
