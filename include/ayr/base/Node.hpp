#ifndef AYR_BASE_NODE_HPP
#define AYR_DETIAL_NODE_HPP

#include "raise_error.hpp"
#include "ayr_traits.hpp"
#include "IteratorInfo.hpp"

namespace ayr
{
	// 简单节点类型，值能前向移动
	template<typename T>
	class SimpleNode : public Object<SimpleNode<T>>
	{
	public:
		using Value_t = T;

		using self = SimpleNode<Value_t>;
	public:
		SimpleNode() : value_(), next_(nullptr) {}

		SimpleNode(const Value_t& value) : value_(value), next_(nullptr) {}

		SimpleNode(Value_t&& value) : value_(std::move(value)), next_(nullptr) {}

		SimpleNode(const self& other) : value_(other.value_), next_(nullptr) {}

		SimpleNode(self&& other) : value_(std::move(other.value_)), next_(std::exchange(other.next, nullptr)) {}

		// 只拷贝值，不拷贝连接的节点
		self& operator=(const self& other)
		{
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other)
		{
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// 获取值
		Value_t& get() { return value_; }

		const Value_t& get() const { return value_; }

		// 判断是否有下一节点
		bool has_next() const { return next_ != nullptr; }

		// 获取下一节点
		self* next_node() const { return next_; }

		// 设置并获取下一节点
		self* next_node(self* node) { return next_ = node; }

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<Node  " << get() << ">";

			return stream.str();
		}
	protected:
		Value_t value_;

		self* next_;
	};


	// 双向节点类型，值能前后移动
	template<typename T>
	class BiSimpleNode : public Object<BiSimpleNode<T>>
	{
	public:
		using Value_t = T;

		using self = BiSimpleNode<T>;
	public:
		BiSimpleNode() : value_(), prev_(nullptr), next_(nullptr) {}

		BiSimpleNode(const T& value) : value_(value), prev_(nullptr), next_(nullptr) {}

		BiSimpleNode(T&& value) : value_(std::move(value)), prev_(nullptr), next_(nullptr) {}

		BiSimpleNode(const self& other) : value_(std::move(other.value_)), prev_(nullptr), next_(nullptr) {}

		BiSimpleNode(self&& other) : value_(std::move(other.value_)), prev_(std::exchange(other.prev_, nullptr)), next_(std::exchange(other.next_, nullptr)) {}

		self& operator=(const self& other)
		{
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other)
		{
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		Value_t& get() { return value_; }

		const Value_t& get() const { return value_; }

		// 判断是否有前节点
		bool has_prev() const { return prev_ != nullptr; }

		// 判断是否有后节点
		bool has_next() const { return next_ != nullptr; }

		// 获取前节点
		self* prev_node() const { return prev_; }

		// 获取前节点
		self* prev_node(self* node)
		{
			prev_ = node;
			if (node)
				node->next_ = this;
			return prev_;
		}

		// 获取后节点，如果没有后节点，未定义行为
		self* next_node() const { return next_; }

		// 获取后节点
		self* next_node(self* node)
		{
			next_ = node;
			if (node)
				node->prev_ = this;
			return next_;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<BiNode  " << get() << ">";

			return stream.str();
		}
	protected:
		self* prev_, * next_;

		Value_t value_;
	};


	template<bool IsConst, bool IsBidirectional, typename ND>
	struct NodeIterator : public IteratorInfo<NodeIterator<IsConst, IsBidirectional, ND>, NonContainer, std::conditional_t<IsBidirectional, std::bidirectional_iterator_tag, std::forward_iterator_tag>, add_const_t<IsConst, typename ND::Value_t>>
	{
		using self = NodeIterator<IsConst, IsBidirectional, ND>;

		using ItInfo = IteratorInfo<self, NonContainer, std::conditional_t<IsBidirectional, std::bidirectional_iterator_tag, std::forward_iterator_tag>, add_const_t<IsConst, typename ND::Value_t>>;

		using Node_t = ND;

		NodeIterator() : node_(nullptr) {}

		NodeIterator(Node_t* node) : node_(node) {}

		NodeIterator(const self& other) : node_(other.node_) {}

		self& operator=(const self& other) { node_ = other.node_; return *this; }

		typename ItInfo::reference operator*() const
		{
			if (node_ == nullptr)
				NullPointerError("Dereference null pointer");

			return node_->get();
		}

		typename ItInfo::pointer operator->() const
		{
			if (node_ == nullptr)
				NullPointerError("Dereference null pointer");

			return &node_->get();
		}

		self& operator++() { node_ = node_->next_node(); return *this; }

		self operator++(int) { self tmp(*this); ++*this; return tmp; }

		self& operator--() requires IsBidirectional { node_ = node_->prev_node(); return *this; }

		self operator--(int) requires IsBidirectional { self tmp(*this); --*this; return tmp; }

		self operator+(ItInfo::difference_type n) const
		{
			Node_t* tmp = node_;
			while (n--) tmp = tmp->next_node();
			return tmp;
		}

		self operator-(ItInfo::difference_type n) const requires IsBidirectional
		{
			Node_t* tmp = node_;
			while (n--) tmp = tmp->prev_node();
			return tmp;
		}

		self& operator+=(ItInfo::difference_type n)
		{
			while (n--) node_ = node_->next_node();
			return *this;
		}

		self& operator-=(ItInfo::difference_type n) requires IsBidirectional
		{
			while (n--) node_ = node_->prev_node();
			return *this;
		}

		typename ItInfo::difference_type operator-(const self& other) const
		{
			Node_t* tmp = node_;
			typename ItInfo::difference_type n = 0;
			while (tmp != other.node_)
			{
				tmp = tmp->next_node();
				if (tmp == nullptr)
					RuntimeError("cannot calculate distance between two iterators");
				++n;
			}
			return n;
		}

		bool __equals__(const self& other) const { return node_ == other.node_; }
	private:
		Node_t* node_;
	};
}

#endif