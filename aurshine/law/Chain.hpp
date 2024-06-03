#pragma once
#include <law/printer.hpp>

template<typename Node>
class Chain: public Object
{
public:
	Chain() : head_(nullptr), size_(0) {}


private:
	Node* head_;	
	
	c_size size_;
};