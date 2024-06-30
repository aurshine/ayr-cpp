#pragma once
#include <law/Node.hpp>
#include <law/ayr_memory.hpp>


namespace ayr
{
	// 链迭代器
	template<NodeTypeConcept Node>
	class ChainIterator;

	// 双向链迭代器
	template<BiNodeTypeConcept BiNode>
	class BiChainIterator;


	template<NodeTypeConcept Node, typename C = Creator<Node>>
	class SimpleChain : public Object
	{
		friend class ChainIterator<Node>;
	public:
		SimpleChain() : head_(nullptr), tail_(nullptr), size_(0) {}

		c_size size() const { return size_; }

		bool contains(const Node& node) const
		{
			for (auto& current : *this)
				if (current == node)
					return true;
			return false;
		}

		template<typename... Args>
		void append(Args&& ...args)
		{
			Node* new_node = creator_.create(std::forward<Args>(args)...);

			if (size_ == 0)
				head_ = new_node;
			else
				tail_->next = new_node;

			tail_ = new_node;
			size_++;
		}

		Node& operator[](c_size index)
		{
			assert_insize(index, -size_, size_ - 1);
			if (index == -1)	return *tail_;

			index = (index + size_) % size_;

			Node* current = head_;
			while (index--) current = current->next;

			return *current;
		}

		const Node& operator[](c_size index) const
		{
			assert_insize(index, -size_, size_ - 1);
			if (index == -1)	return *tail_;

			index = (index + size_) % size_;

			Node* current = head_;
			while (index--) current = current->next;

			return *current;
		}

		Array<Node> to_array() const
		{
			Array<Node> ret(size_);

			c_size pos = 0;
			for (auto& current : *this)
				ret[pos++] = current;

			return ret;
		}

		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<Chain> [";
			for (auto& current : *this)
			{
				stream << current.value;
				if (current.next != nullptr)
					stream << " -> ";
			}
			stream << "]";

			memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
			return __str_buffer__;
		}

		ChainIterator<Node> begin() const { return ChainIterator<Node>{this->head_}; }

		ChainIterator<Node> end() const { return ChainIterator<Node>{nullptr}; }

	protected:
		Node* head_, * tail_;

		c_size size_;

		C creator_;
	};


	template<BiNodeTypeConcept BiNode, typename C = Creator<BiNode>>
	class BiSimpleChain : public SimpleChain<BiNode, C>
	{
		friend class BiChainIterator<BiNode>;

		using super = SimpleChain<BiNode, C>;

	public:
		BiSimpleChain() : SimpleChain<BiNode, C>() {}

		template<typename... Args>
		void append(Args&& ...args)
		{
			BiNode* new_node = super::creator_.create(std::forward<Args>(args)...);
			if (super::size_ == 0)
				super::head_ = new_node;
			else
			{
				super::tail_->next = new_node;
				new_node->prev = super::tail_;
			}

			super::tail_ = new_node;
			++super::size_;
		}

		template<typename... Args>
		void prepend(Args&& ...args)
		{
			BiNode* new_node = super::creator_.create(std::forward<Args>(args)...);
			if (super::size_ == 0)
				super::tail_ = new_node;
			else
			{
				new_node->next = super::head_;
				super::head_->prev = new_node;
			}
			super::head_ = new_node;
			++super::size_;
		}
		const char* __str__() const
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

			memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
			return __str_buffer__;
		}

		BiChainIterator<BiNode> begin() const { return BiChainIterator<BiNode>{super::head_}; }

		BiChainIterator<BiNode> end() const { return BiChainIterator<BiNode>{nullptr}; }

		std::reverse_iterator<BiChainIterator<BiNode>> rbegin() const { return BiChainIterator<BiNode>{super::tail_}; }

		std::reverse_iterator<BiChainIterator<BiNode>> rend() const { return BiChainIterator<BiNode>{nullptr}; }
	};


	// Chain迭代器
	template<NodeTypeConcept Node>
	class ChainIterator : public Object
	{
	public:
		ChainIterator(Node* node = nullptr) : current_(node) {}

		ChainIterator(const ChainIterator& other) : current_(other.current_) {}

		Node& operator*() const { return *current_; }

		Node* operator->() const { return current_; }

		ChainIterator& operator++()
		{
			current_ = current_->next;
			return *this;
		}

		ChainIterator operator++(int)
		{
			ChainIterator temp{ current_ };
			current_ = current_->next;
			return temp;
		}

		cmp_t __cmp__(const ChainIterator& other) const { return current_ - other.current_; }

		bool operator!=(const ChainIterator& other) const { return __cmp__(other); }

	protected:
		Node* current_;
	};


	// BiChain迭代器
	template<BiNodeTypeConcept BiNode>
	class BiChainIterator : public ChainIterator<BiNode>
	{
	public:
		BiChainIterator(BiNode* node = nullptr) : ChainIterator<BiNode>(node) {}

		BiChainIterator(const BiChainIterator& other) : ChainIterator<BiNode>(other) {}

		BiChainIterator& operator--()
		{
			this->current_ = this->current_->prev;
			return *this;
		}

		BiChainIterator operator--(int)
		{
			BiChainIterator temp{ this->current_ };
			this->current_ = this->current_->prev;
			return temp;
		}

		cmp_t __cmp__(const BiChainIterator& other) const { return this->current_ - other.current_; }

		bool operator!=(const BiChainIterator& other) const { return __cmp__(other); }
	};


	template<typename T>
	using Chain = SimpleChain<SimpleNode<T>, Creator<SimpleNode<T>>>;


	template<typename T>
	using BiChain = BiSimpleChain<BiSimpleNode<T>, Creator<BiSimpleNode<T>>>;
}
