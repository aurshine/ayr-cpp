#ifndef AYR_LAW_DETIAL_RELATIONITERATOR_HPP
#define AYR_LAW_DETIAL_RELATIONITERATOR_HPP

#include <law/detail/object.hpp>
#include <law/detail/ayr_concepts.hpp>

namespace ayr
{
	template<IteratorLike I>
	class SelfAddMove
	{
		using self = SelfAddMove;
	public:
		// 移动到下一个元素， 修改自身
		static I& next(I& iter) { return ++iter; }

		// 移动到上一个元素， 修改自身
		static I& prev(I& iter) { return --iter; }
	};


	template<IteratorLike I, typename IterMove>
	class RelationIterator : Object<RelationIterator<I, IterMove>>
	{
		using self = RelationIterator<I, IterMove>;

		using super = Object<self>;

	public:
		using Value_t = std::remove_reference_t<decltype(*std::declval<I>())>;

		using Iter_t = I;

		using Move_t = IterMove;

		RelationIterator(const Iter_t& iter) : iter_(iter) {}

		RelationIterator(const self& other) : iter_(other.iter_) {};

		RelationIterator(self&& other) : iter_(std::move(other.iter_)) {}

		Value_t& operator*() { return *iter_; }

		const Value_t& operator*() const { return *iter_; }

		Value_t* operator->() { return &*iter_; }

		const Value_t* operator->() const { return &*iter_; }

		self& operator++()
		{
			Move_t::next(iter_);
			return *this;
		}

		self operator++ (int)
		{
			Iter_t temp = iter_;
			++*this;
			return self(temp);
		}

		self& operator--()
		{
			iter_ = Move_t::prev(iter_);
			return *this;
		}

		self operator--(int)
		{
			Iter_t temp = iter_;
			--*this;
			return self(temp);
		}

		bool __equals__(const self& other) const override
		{
			return iter_ == other.iter_;
		}
	private:
		Iter_t iter_;
	};
}
#endif