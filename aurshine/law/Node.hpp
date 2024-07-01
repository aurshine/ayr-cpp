#pragma once
#include <law/printer.hpp>
#include <law/DynArray.hpp>


namespace ayr
{
	// Node���͸���Լ��
	template<typename T>
	concept NodeTypeConcept = requires (T a, const T & b, T&& c)
	{
		{ T() } -> std::same_as<T>;
		{ a = b } -> std::same_as<T&>;
		{ T(b) } -> std::same_as<T>;
		{ a = std::move(c) } -> std::same_as<T&>;
		{ T(std::move(c)) } -> std::same_as<T>;
		{ a.next };
		{a.value};
	};


	// BiNode���͸���Լ��
	template<typename T>
	concept BiNodeTypeConcept = NodeTypeConcept<T> && requires (T a) {{ a.prev };};


	// �򵥽ڵ����ͣ�ֵ��ǰ���ƶ�
	template<typename T>
	class SimpleNode: public Object
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

		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<Node  " << value << ">";
			
			memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
			return __str_buffer__;
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


	// ˫��ڵ����ͣ�ֵ��ǰ���ƶ�
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

		const char* __str__() const
		{
			std::stringstream stream;
			stream << "<BiNode  " << value << ">";

			return memcpy__str_buffer__(stream.str().c_str(), stream.str().size());
		}

		T value;

		BiSimpleNode* prev, * next;
	};
}