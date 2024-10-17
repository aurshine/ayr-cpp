#ifndef AYR_LAW_DETIAL_RELATIONITERATOR_HPP
#define AYR_LAW_DETIAL_RELATIONITERATOR_HPP

#include <law/detail/object.hpp>
#include <law/detail/ayr_concepts.hpp>
#include <law/detail/printer.hpp>


namespace ayr
{
	template<IteratorLike I>
	class IterMoveImpl : public Object<IterMoveImpl<I>>
	{
	public:
		using Iterator = I;

		using Value_t = std::remove_reference_t<decltype(*std::declval<Iterator>())>;

		using Reference_t = Value_t&;

		static Iterator& next(Iterator& iter) { NotImplementedError("next not implemented"); return None<I>; }

		static Iterator& prev(Iterator& iter) { NotImplementedError("prev not implemented"); return None<I>; }
	};


	template<IteratorLike I>
	class SelfAddMove : public IterMoveImpl<I>
	{
		using self = SelfAddMove;
	public:
		// 移动到下一个元素， 修改自身
		static I& next(I& iter) { return ++iter; }

		// 移动到上一个元素， 修改自身
		static I& prev(I& iter) { return --iter; }
	};


	template<typename IterMove>
	class RelationIterator : public Object<RelationIterator<IterMove>>
	{
		using self = RelationIterator<IterMove>;

		using super = Object<self>;

	public:
		using Iterator = typename IterMove::Iterator;

		using Value_t = typename IterMove::Value_t;

		using Reference_t = typename IterMove::Reference_t;

		using Move_t = IterMove;

		RelationIterator(const Iterator& iter) : iter_(iter) {}

		RelationIterator(const self& other) : iter_(other.iter_) {};

		RelationIterator(self&& other) : iter_(std::move(other.iter_)) {}

		Reference_t operator*() { return *iter_; }

		const Reference_t operator*() const { return *iter_; }

		Value_t* operator->() { return &(*iter_); }

		const Value_t* operator->() const { return &(*iter_); }

		self& operator++()
		{
			Move_t::next(iter_);
			return *this;
		}

		self operator++ (int)
		{
			Iterator temp = iter_;
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
			Iterator temp = iter_;
			--*this;
			return self(temp);
		}

		bool __equals__(const self& other) const override
		{
			return iter_ == other.iter_;
		}
	private:
		Iterator iter_;
	};
}
#endif