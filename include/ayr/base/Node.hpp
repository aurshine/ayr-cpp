#ifndef AYR_BASE_NODE_HPP
#define AYR_BASE_NODE_HPP

#include "raise_error.hpp"
#include "ayr_traits.hpp"
#include "IteratorInfo.hpp"

namespace ayr
{
	// 简单节点类型，值能前向移动
	template<typename T>
	class SimpleNode : public IteratorInfo<SimpleNode<T>, NonContainer, std::forward_iterator_tag, T>
	{
	public:
		using self = SimpleNode<T>;

		using ItInfo = IteratorInfo<self, NonContainer, std::forward_iterator_tag, T>;

		// 默认构造函数会构造一个空节点，通常不使用
		constexpr SimpleNode() : value_(nullptr), next_(nullptr) {}

		SimpleNode(typename ItInfo::const_reference value) : value_(ayr_cove_make(value)), next_(none_node()) {}

		SimpleNode(typename ItInfo::rvalue_reference value) : value_(ayr_cove_make(std::move(value))), next_(none_node()) {}

		SimpleNode(const self& other) : value_(other.value_), next_(other.next_) {}

		self& operator=(const self& other) { value_ = other.value_; next_ = other.next_; return *this; }

		typename ItInfo::reference operator*() const { return *value_; }

		typename ItInfo::pointer operator->() const { return value_; }

		self& operator++() { return *this = *next_; }

		self operator++(int) { self tmp(*this); ++*this; return tmp; }

		self operator+(ItInfo::difference_type n) const { return std::next(*this, n); }

		self& operator+=(ItInfo::difference_type n) { std::advance(*this, n); return *this; }

		typename ItInfo::difference_type operator-(const self& other) const { return std::distance(other, *this); }

		bool __equals__(const self& other) const { return value_ == other.value_ && next_ == other.next_; }

		// 空节点
		constexpr static self* none_node()
		{
			static self* none_node_ = ayr_make<self>();
			return none_node_;
		}

		// 释放分配的内存
		void destroy()
		{
			ayr_desloc(value_, 1);
			std::memset(this, 0, sizeof self);
		}

		// 判断是否是空节点
		bool is_none_node() const { return value_ == nullptr && next_ == nullptr; }

		// 判断是否有下一节点
		bool has_next() const { return !(next_ == nullptr || next_->is_none_node()); }

		// 获取下一节点
		self* next_node() const { return next_; }

		// 设置并获取下一节点
		self* next_node(self* node) { return next_ = node; }

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<Node  " << **this << ">";

			return stream.str();
		}
	private:
		typename ItInfo::pointer value_;

		self* next_;
	};

	// 双向节点类型，值能前后移动
	template<typename T>
	class BiSimpleNode : public IteratorInfo<BiSimpleNode<T>, NonContainer, std::bidirectional_iterator_tag, T>
	{
	public:
		using self = BiSimpleNode<T>;

		using ItInfo = IteratorInfo<self, NonContainer, std::bidirectional_iterator_tag, T>;

		// 默认构造函数会构造一个空节点，通常不使用
		constexpr BiSimpleNode() : value_(nullptr), prev_(nullptr), next_(nullptr) {}

		BiSimpleNode(typename ItInfo::const_reference value) : value_(ayr_cove_make(value)), prev_(none_node()), next_(none_node()) {}

		BiSimpleNode(typename ItInfo::rvalue_reference value) : value_(ayr_cove_make(std::move(value))), prev_(none_node()), next_(none_node()) {}

		BiSimpleNode(const self& other) :
			value_(other.value_),
			prev_(other.prev_),
			next_(other.next_) {}

		self& operator=(const self& other)
		{
			value_ = other.value_;
			prev_ = other.prev_;
			next_ = other.next_;
			return *this;
		}

		typename ItInfo::reference operator*() const { return *value_; }

		typename ItInfo::pointer operator->() const { return value_; }

		self& operator++ () { return *this = *next_; }

		self operator++(int) { self tmp(*this); ++*this; return tmp; }

		self& operator--() { return *this = *prev_; }

		self operator--(int) { self tmp(*this); --*this; return tmp; }

		self operator+(ItInfo::difference_type n) const { return std::next(*this, n); }

		self& operator+=(ItInfo::difference_type n) { std::advance(*this, n); return *this; }

		self operator-(ItInfo::difference_type n) const { return std::prev(*this, n); }

		self& operator-=(ItInfo::difference_type n) { std::advance(*this, -n); return *this; }

		typename ItInfo::difference_type operator-(const self& other) const { return std::distance(other, *this); }

		bool __equals__(const self& other) const { return value_ == other.value_ && prev_ == other.prev_ && next_ == other.next_; }

		// 空节点
		constexpr static self* none_node()
		{
			static self* none_node_ = ayr_make<self>();
			return none_node_;
		}

		// 释放分配的内存
		void destroy()
		{
			ayr_desloc(value_, 1);
			std::memset(this, 0, sizeof self);
		}

		// 判断是否是空节点
		bool is_none_node() const { return value_ == nullptr && prev_ == nullptr && next_ == nullptr; }

		// 判断是否有前节点
		bool has_prev() const { return !(prev_ == nullptr || prev_->is_none_node()); }

		// 判断是否有后节点
		bool has_next() const { return !(next_ == nullptr || next_->is_none_node()); }

		// 获取前节点
		self* prev_node() const { return prev_; }

		// 获取前节点
		self* prev_node(self* node)
		{
			prev_ = node;
			if (!node->is_none_node())
				node->next_ = this;
			return prev_;
		}

		// 获取后节点，如果没有后节点，未定义行为
		self* next_node() const { return next_; }

		// 获取后节点
		self* next_node(self* node)
		{
			next_ = node;
			if (!node->is_none_node())
				node->prev_ = this;
			return next_;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<BiNode  " << **this << ">";

			return stream.str();
		}
	private:
		typename ItInfo::pointer value_;

		self* prev_, * next_;
	};
}

#endif