#ifndef AYR_DETAIL_NODE_HPP
#define AYR_DETIAL_NODE_HPP

#include <ayr/detail/printer.hpp>


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
	class SimpleNode : public Object<SimpleNode<T>>
	{
	public:
		using Value_t = T;

		using self = SimpleNode<T>;
	public:
		SimpleNode() : value(), next(nullptr) {}

		SimpleNode(const T& value) : value(value), next(nullptr) {}

		SimpleNode(T&& value) : value(std::move(value)), next(nullptr) {}

		SimpleNode(const self& other) : value(other.value), next(nullptr) {}

		SimpleNode(self&& other) : value(std::move(other.value)), next(other.next) { other.next = nullptr; }

		self& operator=(const self& other)
		{
			value = other.value;
			next = nullptr;
			return *this;
		}

		self& operator=(self&& other)
		{
			value = std::move(other.value);
			next = other.next;
			other.next = nullptr;
			return *this;
		}

		CString __str__() const override
		{
			std::stringstream stream;
			stream << "<Node  " << value << ">";

			return CString(stream.str());
		}

		T value;

		self* next;
	};


	// 双向节点类型，值能前后移动
	template<typename T>
	class BiSimpleNode : public Object<BiSimpleNode<T>>
	{
	public:
		using Value_t = T;

		using self = BiSimpleNode<T>;
	public:
		BiSimpleNode() : value(), prev(nullptr), next(nullptr) {}

		BiSimpleNode(const T& value) : value(value), prev(nullptr), next(nullptr) {}

		BiSimpleNode(T&& value) : value(std::move(value)), prev(nullptr), next(nullptr) {}

		BiSimpleNode(const self& other) : value(other.value), prev(nullptr), next(nullptr) {}

		BiSimpleNode(self&& other) : value(std::move(other.value)), prev(other.prev), next(other.next) { other.prev = other.next = nullptr; }

		self& operator=(const self& other)
		{
			value = other.value;
			prev = nullptr;
			next = nullptr;
			return *this;
		}

		self& operator=(self&& other)
		{
			value = std::move(other.value);
			prev = other.prev;
			next = other.next;
			other.prev = other.next = nullptr;
			return *this;
		}

		CString __str__() const override
		{
			std::stringstream stream;
			stream << "<BiNode  " << value << ">";

			return CString(stream.str());
		}

		T value;

		self* prev, * next;
	};
}

#endif