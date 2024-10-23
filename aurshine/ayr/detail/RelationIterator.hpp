#ifndef AYR_LAW_DETIAL_RELATIONITERATOR_HPP
#define AYR_LAW_DETIAL_RELATIONITERATOR_HPP

#include <ayr/detail/object.hpp>
#include <ayr/detail/ayr_concepts.hpp>
#include <ayr/detail/printer.hpp>


namespace ayr
{
	template<std::input_or_output_iterator I>
	class IterMoveImpl : public Object<IterMoveImpl<I>>
	{
	public:
		using Iterator = I;

		using iterator_category = std::bidirectional_iterator_tag;

		using value_type = std::remove_reference_t<decltype(*std::declval<Iterator>())>;

		static Iterator& next(Iterator& iter) { NotImplementedError("next not implemented"); return None<I>; }

		static Iterator& prev(Iterator& iter) { NotImplementedError("prev not implemented"); return None<I>; }
	};


	template<std::input_or_output_iterator I>
	class SelfAddMove : public IterMoveImpl<I>
	{
		using self = SelfAddMove;

		using super = IterMoveImpl<I>;
	public:
		using Iterator = I;

		using iterator_category = std::bidirectional_iterator_tag;

		using value_type = std::remove_reference_t<decltype(*std::declval<Iterator>())>;

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

		using Move_t = IterMove;

		using iterator_category = typename IterMove::iterator_category;

		using value_type = typename IterMove::value_type;

		using difference_type = std::ptrdiff_t;

		using pointer = value_type*;

		using const_pointer = const value_type*;

		using reference = value_type&;

		using const_reference = const value_type&;

		RelationIterator() : iter_() {};

		RelationIterator(const Iterator& iter) : iter_(iter) {}

		RelationIterator(const self& other) : iter_(other.iter_) {};

		RelationIterator(self&& other) : iter_(std::move(other.iter_)) {}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			iter_ = other.iter_;
			return *this;
		}

		reference operator*() { return *iter_; }

		const_reference operator*() const { return *iter_; }

		pointer operator->() { return &(*iter_); }

		const_pointer operator->() const { return &(*iter_); }

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