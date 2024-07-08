#pragma once
#include <law/printer.hpp>
#include <law/Array.hpp>

namespace ayr
{
    // EXP[i] 第i位为1其余位为0
    constexpr static auto EXP2 = make_stl_array<c_size, 64>([](int x) { return 1ll << x; });


    template<typename B>
    inline B lowbit(const B& x)
    {
        return x & -x;
    }


    template<typename B>
    inline int lowbit_index(const B& x)
    {
        int l = 0;
        while ((x >> l & 1) == 0) ++l;
        return l;
    }


    template<typename B>
    inline int highbit_index(const B& x)
    {
        int l = 0;
        while (x >> l) ++l;
        
        return --l;
    }


    template<typename B>
    inline B highbit(const B& x)
    {
        return x >> highbit_index(x);
    }
}
