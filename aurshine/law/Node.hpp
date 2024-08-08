#ifndef AYR_LAW_NODE_HPP
#define AYR_LAW_NODE_HPP

#include <law/detail/printer.hpp>
#include <law/DynArray.hpp>


namespace ayr
{
	// Node类型概念约束
	template<typename T>
	concept NodeTypeConcept = requires (T a, const T & b, T && c)
	{
		{ T() } -> std::same_as<T>;
		{ a = b } -> std::same_as<T&>;
		{ T(b) } -> std::same_as<T>;
		{ a = std::move(c) } -> std::same_as<T&>;
		{ T(std::move(c)) } -> std::same_as<T>;
		{ a.next };
		{ a.value };
	};


	// BiNode类型概念约束
	template<typename T>
	concept BiNodeTypeConcept = NodeTypeConcept<T> && requires (T a) { { a.prev }; };


	// 简单节点类型，值能前向移动
	template<typename T>
	class SimpleNode : public Object
	{
	public:
		using Value_t = T;

	public:
		SimpleNode() : value(), next(nullptr) {}

		SimpleNode(const T& value) : value(value), next(nullptr) {}

		SimpleNode(T&& value) : value(std::move(value)), next(nullptr) {}

		SimpleNode(const SimpleNode& other) : value(other.value), next(nullptr) {}

		SimpleNode(SimpleNode&& other) : value(std::move(other.value)), next(other.next) { other.next = nullptr; }

		SimpleNode& operator=(const SimpleNode& other)
		{
			value = other.value;
			next = nullptr;
			return *this;
		}

		SimpleNode& operator=(SimpleNode&& other)
		{
			value = std::move(other.value);
			next = other.next;
			other.next = nullptr;
			return *this;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<Node  " << value << ">";

			return CString(stream.str());
		}

		cmp_t __cmp__(const SimpleNode& other)
		{
			if (value < other.value) return -1;
			if (value > other.value) return 1;
			return 0;
		}

		T value;

		SimpleNode* next;
	};


	// 双向节点类型，值能前后移动
	template<typename T>
	class BiSimpleNode : public Object
	{
	public:
		using Value_t = T;

	public:
		BiSimpleNode() : value(), prev(nullptr), next(nullptr) {}

		BiSimpleNode(const T& value) : value(value), prev(nullptr), next(nullptr) {}

		BiSimpleNode(T&& value) : value(std::move(value)), prev(nullptr), next(nullptr) {}

		BiSimpleNode(const BiSimpleNode& other) : value(other.value), prev(nullptr), next(nullptr) {}

		BiSimpleNode(BiSimpleNode&& other) : value(std::move(other.value)), prev(other.prev), next(other.next) { other.prev = other.next = nullptr; }

		BiSimpleNode& operator=(const BiSimpleNode& other)
		{
			value = other.value;
			prev = nullptr;
			next = nullptr;
			return *this;
		}

		BiSimpleNode& operator=(BiSimpleNode&& other)
		{
			value = std::move(other.value);
			prev = other.prev;
			next = other.next;
			other.prev = other.next = nullptr;
			return *this;
		}

		CString __str__() const
		{
			std::stringstream stream;
			stream << "<BiNode  " << value << ">";

			return CString(stream.str());
		}

		T value;

		BiSimpleNode* prev, * next;
	};
}

#endif