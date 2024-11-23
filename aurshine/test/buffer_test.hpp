#pragma once

#include <ayr/base/buffer.hpp>

void buffer_test()
{
	using ayr::Buffer;
	using ayr::print;

	Buffer<int> buffer(10);
	for (int i = 0; i < 10; i++)
	{
		buffer.append(i);
	}

	print("buffer", buffer);
	print("buffer.to_array()", buffer.to_array());
	print("buffer", buffer);
	print("buffer.move_array()", buffer.move_array());
	print("buffer", buffer);
}