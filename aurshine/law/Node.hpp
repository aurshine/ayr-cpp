#pragma once
#include <law/printer.hpp>
#include <law/DynArray.hpp>


namespace ayr
{
	// Node类型概念约束
	template<typename T>
	concept NodeTypeConcept = requires (T a, const T & b, T&& c)
	{
		{ T() } -> std::same_as<T>;
		{ a = b } -> std::same_as<T&>;
		{ T(b) } -> std::same_as<T>;
		{ a = std::move(c) } -> std::same_as<T&>;
		{ T(std::move(c)) } -> std::same_as<T>;
		{ a.next };
	};


	// BiNode类型概念约束
	template<typename T>
	concept BiNodeTypeConcept = NodeTypeConcept<T> && requires (T a, const T & b, T&& c) {{ a.prev };};


	template<typename T>
	class SampleNode: Object
	{
	public:
		SampleNode() : value(), next(nullptr) {}

		SampleNode(const T& value) : value(value), next(nullptr) {}

		SampleNode(T&& value) : value(std::move(value)), next(nullptr) {}

		SampleNode(const SampleNode& other) : value(other.value), next(nullptr) {}

		SampleNode(SampleNode&& other) : value(std::move(other.value)), next(other.next) { other.next = nullptr; }

		SampleNode& operator=(const SampleNode& other)
		{
			value = other.value;
			next = nullptr;
			return *this;
		}

		SampleNode& operator=(SampleNode&& other)
		{
			value = std::move(other.value);
			next = other.next;
			other.next = nullptr;
			return *this;
		}

		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<Node> " << value;
			
			memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
			return __str_buffer__;
		}

		T value;

		SampleNode* next;
	};

	template<typename T>
	class BiSampleNode : Object
	{
	public:
		BiSampleNode() : value(), prev(nullptr), next(nullptr) {}

		BiSampleNode(const T& value) : value(value), prev(nullptr), next(nullptr) {}

		BiSampleNode(T&& value) : value(std::move(value)), prev(nullptr), next(nullptr) {}

		BiSampleNode(const BiSampleNode& other) : value(other.value), prev(nullptr), next(nullptr) {}

		BiSampleNode(BiSampleNode&& other) : value(std::move(other.value)), prev(other.prev), next(other.next) { other.prev = other.next = nullptr; }

		BiSampleNode& operator=(const BiSampleNode& other)
		{
			value = other.value;
			prev = nullptr;
			next = nullptr;
			return *this;
		}

		BiSampleNode& operator=(BiSampleNode&& other)
		{
			value = std::move(other.value);
			prev = other.prev;
			next = other.next;
			other.prev = other.next = nullptr;
			return *this;
		}

		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<BiNode> " << value;

			memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
			return __str_buffer__;
		}

		T value;

		BiSampleNode* prev, * next;
	};

	template<NodeTypeConcept Node>
	class NodeCreator : Object
	{
	public:
		template<typename... Args>
		Node* create(Args&& ... args)
		{
			nodes_.append(Node(std::forward<Args>(args)...));

			return &nodes_[-1];
		}

	private:
		DynArray<Node> nodes_;
	};


}