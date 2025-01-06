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

		using Iterator = NodeIterator<false, Node_t>;

		using ConstIterator = NodeIterator<true, Node_t>;

		Chain() : head_(nullptr), tail_(nullptr), size_(0) {}

		c_size size() const { return size_; }

		Value_t& at(c_size index)
		{
			Node_t* const cur = head_;
			while (index-- && cur)
				cur = cur->next_node();
			return cur->get();
		}

		const Value_t& at(c_size index) const
		{
			Node_t* const cur = head_;
			while (index-- && cur)
				cur = cur->next_node();
			return cur->get();
		}

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

		Array<Value_t> to_array() const
		{
			Array<Value_t> ret(size_);

			c_size pos = 0;
			for (auto& current : *this)
				ret[pos++] = current->get();

			return ret;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<Chain> [";
			for (auto& current : *this)
			{
				stream << current.get();
				if (current.has_next())
					stream << " -> ";
			}
			stream << "]";

			return stream.str();
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
		BiChain() : head_(nullptr), tail_(nullptr), size_(0) {}

		template<typename... Args>
		void prepend(Args&& ...args)
		{
			Node_t* new_node = super::alloc_.create(std::forward<Args>(args)...);
			if (super::size_ == 0)
				super::tail_ = new_node;
			else
				super::head_.prev_node(new_node);

			super::head_ = new_node;
			++super::size_;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<BiChain> [";
			for (auto& current : *this)
			{
				stream << current.value;
				if (current.next != nullptr)
					stream << " <--> ";
			}
			stream << "]";

			return CString(stream.str());
		}
	};
}
#endif