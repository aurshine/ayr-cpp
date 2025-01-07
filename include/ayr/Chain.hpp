#ifndef AYR_CHAIN_HPP
#define AYR_CHAIN_HPP

#include "base/Node.hpp"
#include "base/Sequence.hpp"
#include "ayr_memory.hpp"


namespace ayr
{
	template<typename ND>
	class ChainImpl : public Sequence<ChainImpl<ND>, typename ND::Value_t>
	{
	public:
		using Value_t = typename ND::Value_t;

		using Node_t = ND;

		using Iterator = NodeIterator<false, false, Node_t>;

		using ConstIterator = NodeIterator<true, false, Node_t>;

		ChainImpl() : head_(nullptr), tail_(nullptr), size_(0) {}

		ChainImpl(std::initializer_list<Value_t>&& il) : ChainImpl()
		{
			for (auto& v : il)
				append(v);
		}

		ChainImpl(const ChainImpl& other) : ChainImpl()
		{
			for (auto& v : other)
				append(v);
		}

		ChainImpl(ChainImpl&& other) :
			head_(std::exchange(other.head_, nullptr)),
			tail_(std::exchange(other.tail_, nullptr)),
			size_(std::exchange(other.size_, 0)),
			alloc_(std::move(other.alloc_)) {}

		~ChainImpl() { clear(); }

		c_size size() const { return size_; }

		// 返回node的索引
		Node_t* at_node(c_size index) const
		{
			Node_t* cur = head_;
			while (index-- && cur)
				cur = cur->next_node();
			return cur;
		}

		Value_t& at(c_size index) { return at_node(index)->get(); }

		const Value_t& at(c_size index) const { return at_node(index)->get(); }

		template<typename... Args>
		void append(Args&& ...args)
		{
			Node_t* new_node = alloc_.create(std::forward<Args>(args)...);

			if (size_ == 0)
				head_ = new_node;
			else
				tail_->next_node(new_node);

			tail_ = new_node;
			size_++;
		}

		// 删除前n个节点
		void pop_front(int n = 1)
		{
			Node_t* del_node = head_;
			size_ -= n;
			while (n--)
			{
				head_ = head_->next_node();
				alloc_.destroy(del_node);
				del_node = head_;
			}
			if (size_ == 0) tail_ = nullptr;
		}

		// 从node节点开始删除，可提供node的前驱节点，若不提供则自行寻找
		void pop_from(Node_t* from_node, int n = 1, Node_t* prev_node = nullptr)
		{
			if (from_node == nullptr)
				return;

			if (from_node == head_)
			{
				pop_front(n);
				return;
			}

			if (prev_node == nullptr)
			{
				prev_node = head_;
				while (prev_node->next_node() != from_node)
					prev_node = prev_node->next_node();
			}

			size_ -= n;
			while (n--)
			{
				Node_t* next_node = from_node->next_node();
				alloc_.destroy(from_node);
				from_node = next_node;
			}

			if (from_node == nullptr)
				tail_ = prev_node;

			prev_node->next_node(from_node);
		}

		// 删除最后n个节点
		void pop_back(int n = 1)
		{
			c_size server_size = size() - n;
			if (server_size <= 0)
				clear();
			else
				pop_from(at_node(server_size), n);
		}

		// 清空链表
		void clear() { pop_from(head_, size_); }

		Array<Value_t> to_array() const
		{
			Array<Value_t> ret(size());

			c_size pos = 0;
			for (auto& current : *this)
				ret[pos++] = current->get();

			return ret;
		}

		Iterator begin() { return Iterator{ head_ }; }

		Iterator end() { return Iterator{ nullptr }; }

		ConstIterator begin() const { return ConstIterator{ head_ }; }

		ConstIterator end() const { return ConstIterator{ nullptr }; }
	protected:
		Node_t* head_, * tail_;

		c_size size_;

		Ayrocator<Node_t> alloc_;
	};

	template<typename T>
	using Chain = ChainImpl<SimpleNode<T>>;


	template<typename T>
	class BiChain : public ChainImpl<BiSimpleNode<T>>
	{
		using Node_t = BiSimpleNode<T>;

		using self = BiChain<T>;

		using super = ChainImpl<Node_t>;
	public:
		using Iterator = NodeIterator<false, true, Node_t>;

		using ConstIterator = NodeIterator<true, true, Node_t>;

		BiChain() : super() {}

		BiChain(std::initializer_list<T>&& il) : super(std::move(il)) {}

		BiChain(const self& other) : super(other) {}

		BiChain(self&& other) : super(std::move(other)) {}

		~BiChain() = default;

		template<typename... Args>
		void prepend(Args&& ...args)
		{
			Node_t* new_node = super::alloc_.create(std::forward<Args>(args)...);
			if (super::size_ == 0)
				super::tail_ = new_node;
			else
				super::head_->prev_node(new_node);

			super::head_ = new_node;
			++super::size_;
		}

		void pop_front(int n = 1)
		{
			super::size_ -= n;
			Node_t* del_node = super::head_;
			while (n--)
			{
				super::head_ = super::head_->next_node();
				super::alloc_.destroy(del_node);
				del_node = super::head_;
			}

			if (super::size_ == 0)
				super::tail_ = nullptr;
			else
				super::head_->prev_node(nullptr);
		}

		// 从node节点开始删除n个节点
		void pop_from(Node_t* from_node, int n = 1)
		{
			if (from_node == nullptr)
				return;

			if (from_node == super::head_)
			{
				pop_front(n);
				return;
			}

			super::size_ -= n;
			Node_t* prev_node = from_node->prev_node();
			while (n--)
			{
				Node_t* next_node = from_node->next_node();
				super::alloc_.destroy(from_node);
				from_node = next_node;
			}

			prev_node->next_node(from_node);
			if (from_node == nullptr)
				super::tail_ = prev_node;
			else
				from_node->prev_node(prev_node);
		}

		Iterator begin() { return Iterator{ super::head_ }; }

		Iterator end() { return Iterator{ nullptr }; }

		ConstIterator begin() const { return ConstIterator{ super::head_ }; }

		ConstIterator end() const { return ConstIterator{ nullptr }; }
	};
}
#endif