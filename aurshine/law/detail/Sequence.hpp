#ifndef AYR_LAW_DETAIL_SEQUENCE_HPP
#define AYR_LAW_DETAIL_SEQUENCE_HPP

#include <law/detail/printer.hpp>
#include <law/detail/IndexIterator.hpp>


namespace ayr
{
	/*
	* Sequence 为线性容器提供基类
	*
	* 成员类型
	*	1. Value_t									表示继承Sequence的容器存储的成员类型
	* 	2. Iterator									表示迭代器类型
	* 	3. ConstIterator							表示常量迭代器类型
	*
	* 提供的接口
	* 	1. operator[](c_size)						访问容器中指定位置的元素，返回引用,索引可以为负数，表示从尾部开始的位置
	*	2. __cmp__(const self& other) const			比较两个Sequence是否相等，返回cmp_t类型
	*	3. __str__() const							打印Sequence的内容，返回CString类型
	*	4. contains(const Value_t& v) const			判断Sequence中是否包含指定元素，返回bool类型
	*	5. find(const Value_t& v) const				查找Sequence中是否存在指定元素，返回ConstIterator类型
	*	6. __iter_container__() const				返回自身，用于实现迭代器相关接口
	*	7. 通过继承IndexContainer<Sequence<T>, T>	实现迭代器相关接口
	*
	* 需要实现的函数(虚函数)
	* 	1. __at__(c_size) const						访问容器中指定位置的元素，返回引用，索引不能为负数
	* 	2. size() const								返回容器中元素的数量
	*/
	template<typename T>
	class Sequence : public IndexContainer<Sequence<T>, T>
	{
	public:
		using self = Sequence<T>;

		using super = IndexContainer<self, T>;
	public:
		using Value_t = T;

		using Iterator = super::Iterator;

		using ConstIterator = super::ConstIterator;

		virtual Value_t& __at__(c_size) { NotImplementedError("Sequence::__at__ not implemented"); return None<Value_t>; }

		virtual const Value_t& __at__(c_size) const { NotImplementedError("Sequence::__at__ const not implemented"); return None<Value_t>; }

		virtual c_size size() const { NotImplementedError("Sequence::size not implemented"); return 0; }

		self& __iter_container__() const { return const_cast<self&>(*this); }

		Value_t& operator[] (c_size index) { return __at__(neg_index(index, size())); }

		const Value_t& operator[] (c_size index) const { return __at__(neg_index(index, size())); }

		cmp_t __cmp__(const self& other) const
		{
			c_size m_size = size(), o_size = other.size();
			for (c_size i = 0; i < m_size && i < o_size; ++i)
				if (operator[](i) < other[i])
					return -1;
				else if (operator[](i) > other[i])
					return 1;
			return m_size - o_size;
		}

		CString __str__() const
		{
			std::stringstream ss;
			ss << "Sequence<" << dtype(Value_t) << ">(";
			for (auto&& value : *this)
				ss << value << ", ";
			ss << ")";
			return CString(ss.str());
		}

		bool contains(const Value_t& v) const { return find_it(v) != super::end(); }

		c_size find(const Value_t& v) const
		{
			for (c_size i = 0, size = size(); i < size; ++i)
				if (__at__(i) == v)
					return i;

			return -1;
		}

		super::Iterator find_it(const Value_t& v)
		{
			for (auto&& it = super::begin(), end = super::end(); it != end; ++it)
				if (*it == v)
					return it;

			return super::end();
		}

		super::ConstIterator find_it(const Value_t& v) const 
		{
			for (auto&& it = super::begin(), end = super::end(); it != end; ++it)
				if (*it == v)
					return it;

			return super::end();
		}
	};
}

#endif