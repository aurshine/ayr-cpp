#ifndef AYR_CHAIN_HPP
#define AYR_CHAIN_HPP

#include "base/Node.hpp"
#include "base/Sequence.hpp"
#include "ayr_memory.hpp"


namespace ayr
{
	// 需要传入一个Node类型
	template<typename ND>
	class ChainImpl : public Sequence<ChainImpl<ND>, typename ND::value_type>
	{
	public:
		using Value_t = typename ND::value_type;

		using Node_t = ND;

		using Iterator = Node_t;

		using ConstIterator = Node_t;

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

		ChainImpl(ChainImpl&& other) :head_(std::exchange(other.head_, nullptr)), tail_(std::exchange(other.tail_, nullptr)), size_(other.size_) {}

		~ChainImpl() { clear(); }

		c_size size() const { return size_; }

		Node_t& at_node_from(const Node_t& from_node, int n = 0)
		{
			return at_node_from_impl(from_node, n);
		}

		const Node_t& at_node_from(const Node_t& from_node, int n = 0) const
		{
			return at_node_from_impl(from_node, n);
		}

		// 返回node的索引
		Node_t& at_node(c_size index) { return at_node_from(*head_, index); }

		const Node_t& at_node(c_size index) const { return at_node_from(*head_, index); }

		Value_t& at(c_size index) { return *at_node(index); }

		const Value_t& at(c_size index) const { return *at_node(index); }

		template<typename... Args>
		Node_t& append(Args&& ...args)
		{
			Node_t* new_node = ayr_make<Node_t>(std::forward<Args>(args)...);

			if (size_ == 0)
				head_ = new_node;
			else
				tail_->next_node(new_node);

			tail_ = new_node;
			size_++;
			return *tail_;
		}

		// 删除前n个节点
		void pop_front(int n = 1)
		{
			size_ -= n;
			Node_t* del_node = head_;
			while (n--)
			{
				head_ = head_->next_node();
				del_node->destroy();
				del_node = head_;
			}
		}

		// 从node节点开始删除，可提供node的前驱节点，若不提供则自行寻找
		void pop_from(const Node_t& from_node, int n = 1, Node_t* prev_node_addr = nullptr)
		{
			if (from_node == *head_)
			{
				pop_front(n);
				return;
			}

			if (prev_node_addr == nullptr)
			{
				prev_node_addr = head_;
				while (*prev_node_addr->next_node() != from_node)
					prev_node_addr = prev_node_addr->next_node();
			}

			size_ -= n;
			Node_t* from_node_addr = prev_node_addr->next_node();
			while (n--)
			{
				Node_t* next_node = from_node_addr->next_node();
				from_node_addr->destroy();
				from_node_addr = next_node;
			}

			if (from_node_addr->is_none_node())
				tail_ = prev_node_addr;

			prev_node_addr->next_node(from_node_addr);
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
		void clear() { pop_front(size()); }

		Array<Value_t> to_array() const
		{
			Array<Value_t> ret(size());

			c_size pos = 0;
			for (auto& current : *this)
				ret[pos++] = current;

			return ret;
		}

		Iterator begin() { return *head_; }

		Iterator end() { return *Node_t::none_node(); }

		ConstIterator begin() const { return *head_; }

		ConstIterator end() const { return *Node_t::none_node(); }
	private:
		// 获取from_node开始第n个节点在当前链表里的地址
		Node_t& at_node_from_impl(const Node_t& from_node, int n = 0) const
		{
			Node_t* cur_node = nullptr;
			if (n == 0)
			{
				cur_node = head_;
				while (cur_node != nullptr && *cur_node != from_node)
					cur_node = cur_node->next_node();
			}
			else
			{
				cur_node = from_node.next_node();
				while (--n)
					cur_node = cur_node->next_node();
			}

			return *cur_node;
		}
	protected:
		Node_t* head_, * tail_;

		c_size size_;
	};

	template<typename T>
	using Chain = ChainImpl<SimpleNode<T>>;


	template<typename T>
	class BiChain : public ChainImpl<BiSimpleNode<T>>
	{
	public:
		using Node_t = BiSimpleNode<T>;

		using self = BiChain<T>;

		using super = ChainImpl<Node_t>;

		BiChain() : super() {}

		BiChain(std::initializer_list<T>&& il) : super(std::move(il)) {}

		BiChain(const self& other) : super(other) {}

		BiChain(self&& other) : super(std::move(other)) {}

		template<typename... Args>
		void prepend(Args&& ...args)
		{
			Node_t* new_node = ayr_make<Node_t>(std::forward<Args>(args)...);
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
				del_node->destroy();
				del_node = super::head_;
			}

			if (super::size_ != 0)
				super::head_->prev_node(Node_t::none_node());
		}

		// 从node节点开始删除n个节点
		void pop_from(const Node_t& from_node, int n = 1)
		{
			if (from_node == *super::head_)
			{
				pop_front(n);
				return;
			}

			super::size_ -= n;
			Node_t* prev_node_addr = from_node.prev_node();
			Node_t* from_node_addr = prev_node_addr->next_node();
			while (n--)
			{
				Node_t* next_node = from_node_addr->next_node();
				from_node_addr->destroy();
				from_node_addr = next_node;
			}

			prev_node_addr->next_node(from_node_addr);
			if (from_node_addr->is_none_node())
				super::tail_ = prev_node_addr;
			else
				from_node_addr->prev_node(prev_node_addr);
		}

		// 删除最后n个节点
		void pop_back(int n = 1)
		{
			c_size server_size = super::size() - n;
			if (server_size <= 0)
				clear();
			else
				pop_from(super::at_node(server_size), n);
		}

		// 清空链表
		void clear() { pop_front(super::size()); }
	};
}
#endif