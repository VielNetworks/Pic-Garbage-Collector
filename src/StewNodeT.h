#pragma once
#ifndef STEWNODET
#define STEWNODET

#include "StewNode.h"

namespace StewGC
{
    template <T>
    class StewNodeT<T>: public StewNode
    {
    public:
      virtual size_t SGCGetByteSize() { return sizeof(T); }
    };
}

#endif // STEWNODET
