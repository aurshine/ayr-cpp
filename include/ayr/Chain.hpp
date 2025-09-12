#ifndef AYR_CHAIN2_HPP
#define AYR_CHAIN2_HPP

#include "base/raise_error.hpp"
#include "base/Sequence.hpp"


namespace ayr
{
	template<typename T>
	class BidirectionalNode : public Object<BidirectionalNode<T>>
	{
		using Value_t = T;

		using self = BidirectionalNode<Value_t>;

		using super = Object<self>;

		self* prev_ = nullptr, * next_ = nullptr;
	public:
		Value_t value;

		BidirectionalNode(const Value_t& value) : value(value) {}

		BidirectionalNode(Value_t&& value) : value(std::move(value)) {}

		BidirectionalNode(self&& other) : value(std::move(other.value)), prev_(other.prev_), next_(other.next_)
		{
			other.prev_ = other.next_ = nullptr;
		}

		self& operator=(self&& other)
		{
			if (this == other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		// 得到当前节点的前一个节点
		self* prev() const { return prev_; }

		// 得到当前节点的下一个节点
		self* next() const { return next_; }

		// 将当前节点设置为前一个节点
		self* prev(self* node)
		{
			prev_ = node;
			if (node) node->next_ = this;
			return node;
		}

		// 将当前节点设置为下一个节点
		self* next(self* node)
		{
			next_ = node;
			if (node) node->prev_ = this;
			return node;
		}
	};

	template<typename T>
	class Chain : public Sequence<Chain<T>, T>
	{
	public:
		using Value_t = T;

		using Node_t = BidirectionalNode<Value_t>;
	private:
		using self = Chain<Value_t>;

		using super = Sequence<self, Value_t>;

		c_size size_;

		Node_t* head_, * tail_;
	public:
		Chain() : size_(0), head_(nullptr), tail_(nullptr) {}

		Chain(const self& other) : Chain()
		{
			for (const Value_t& elem : other)
				append(elem);
		}

		Chain(self&& other) : size_(other.size_), head_(other.head_), tail_(other.tail_)
		{
			other.size_ = 0;
			other.head_ = other.tail_ = nullptr;
		}

		self& operator=(const self& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, other);
		}

		self& operator=(self&& other)
		{
			if (this == &other) return *this;
			ayr_destroy(this);
			return *ayr_construct(this, std::move(other));
		}

		~Chain() { clear(); }

		c_size size() const { return size_; }

		Node_t* at_node(c_size index) const
		{
			Node_t* res = head_;
			while (index--) res = res->next();
			return res;
		}

		Value_t& at(c_size index) { return at_node(index)->value; }

		const Value_t& at(c_size index) const { return at_node(index)->value; }

		Value_t& front() { return head_->value; }

		const Value_t& front() const { return head_->value; }

		Value_t& back() { return tail_->value; }

		const Value_t& back() const { return tail_->value; }

		// 尾部插入一个节点，该节点的生命周期由Chain管理
		Node_t* append_node(Node_t* node)
		{
			if (size_ == 0)
				head_ = node;
			else
				tail_->next(node);
			tail_ = node;
			++size_;
			return node;
		}

		// 尾部插入一个元素
		template<typename... Args>
		Node_t* append(Args&& ...args)
		{
			return append_node(ayr_make<Node_t>(Value_t{ std::forward<Args>(args)... }));
		}

		// 头部插入一个节点，该节点的生命周期由Chain管理
		Node_t* prepend_node(Node_t* node)
		{
			if (size_ == 0)
				tail_ = node;
			else
				node->next(head_);
			head_ = node;
			++size_;
			return node->value;
		}

		// 头部插入一个元素
		template<typename... Args>
		Node_t* prepend(Args&& ...args)
		{
			return prepend_node(ayr_make<Node_t>(Value_t{ std::forward<Args>(args)... }));
		}

		// 删除所有元素
		void clear() { if (size_) pop_range(head_, tail_); }

		// 删除node
		void pop(Node_t* node) { pop_range(node, node); }

		// 删除尾部n个元素
		void pop_back(c_size n = 1)
		{
			Node_t* pos = head_;
			if (n < size_)
				for (c_size i = 0; i < size_ - n; ++i)
					pos = pos->next();

			pop_range(pos, tail_);
		}

		// 删除头部n个元素
		void pop_front(c_size n = 1)
		{
			Node_t* pos = head_;
			if (n < size_)
				for (c_size i = 0; i < n - 1; ++i)
					pos = pos->next();

			pop_range(head_, pos);
		}

		// 删除区间[l, r]内的所有节点
		void pop_range(Node_t* l, Node_t* r)
		{
			if (l == head_)
			{
				if (r == tail_)
					head_ = tail_ = nullptr;
				else
				{
					head_ = r->next();
					head_->prev(nullptr);
				}
			}
			else
			{
				Node_t* l_prev = l->prev();
				l_prev->next(r->next());
				if (r == tail_) tail_ = l_prev;
			}

			Node_t* cur = l;
			while (true)
			{
				--size_;
				Node_t* nxt = cur->next();
				ayr_desloc(cur);

				if (cur == r)
					break;
				else
					cur = nxt;
			}
		}

		template<bool IsConst>
		struct ChainIterator : public IteratorInfo<ChainIterator<IsConst>, NonContainer, std::bidirectional_iterator_tag, add_const_t<IsConst, Value_t>>
		{
			using ItInfo = IteratorInfo<ChainIterator<IsConst>, NonContainer, std::bidirectional_iterator_tag, add_const_t<IsConst, Value_t>>;

			Node_t* node_;
		public:
			ChainIterator() : node_(nullptr) {}

			ChainIterator(Node_t* node) : node_(node) {}

			ChainIterator(const typename ItInfo::iterator_type& other) : node_(other.node_) {}

			ChainIterator& operator=(const typename ItInfo::iterator_type& other) { node_ = other.node_; return *this; }

			ItInfo::reference operator*() const { return node_->value; }

			ItInfo::pointer operator->() const { return &node_->value; };

			typename ItInfo::iterator_type& operator++() { node_ = node_->next(); return *this; }

			typename ItInfo::iterator_type operator++(int) { typename ItInfo::iterator_type res = *this; node_ = node_->next(); return res; }

			typename ItInfo::iterator_type& operator--() { node_ = node_->prev(); return *this; }

			typename ItInfo::iterator_type operator--(int) { typename ItInfo::iterator_type res = *this; node_ = node_->prev(); return res; }

			typename ItInfo::iterator_type operator+(ItInfo::difference_type n) const { typename ItInfo::iterator_type res = *this; return res += n; }

			typename ItInfo::iterator_type operator-(ItInfo::difference_type n) const { typename ItInfo::iterator_type res = *this; return res -= n; }

			typename ItInfo::iterator_type& operator+=(ItInfo::difference_type n)
			{
				while (n--)
					node_ = node_->next();
				return *this;
			}

			typename ItInfo::iterator_type& operator-= (ItInfo::difference_type n)
			{
				while (n--)
					node_ = node_->prev();
				return *this;
			}

			ItInfo::difference_type operator-(const typename ItInfo::iterator_type& other) const
			{
				c_size res = 0;
				Node_t* cur = other.node_;
				while (cur != node_)
				{
					cur = cur->next();
					++res;
				}
				return res;
			}

			bool __equals__(const typename ItInfo::iterator_type& other) const { return node_ == other.node_; }
		};

		using Iterator = ChainIterator<false>;

		using ConstIterator = ChainIterator<true>;

		Iterator begin() { return Iterator(head_); }

		Iterator end() { return Iterator(nullptr); }

		ConstIterator begin() const { return ConstIterator(head_); }

		ConstIterator end() const { return ConstIterator(nullptr); }

		Iterator rbegin() { return Iterator(tail_); }

		Iterator rend() { return Iterator(nullptr); }

		ConstIterator rbegin() const { return ConstIterator(tail_); }

		ConstIterator rend() const { return ConstIterator(nullptr); }
	};

	template<typename T, IteratableU<T> I>
	def chain(I&& it_able)
	{
		Chain<T> res;
		for (auto&& elem : it_able)
			res.append(cond_forward<I>(elem));
		return res;
	}
}
#endif // AYR_CHAIN2_HPP