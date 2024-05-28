#pragma once
#include <law/printer.hpp>

#define no_implement {ayr::error_assert(false, std::format(R"(method {} not implemented)", __func__));}
#define no_implement_v(v) {ayr::error_assert(false, std::format(R"(method {} not implemented)", __func__)); return v;}