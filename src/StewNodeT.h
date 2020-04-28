#pragma once
#ifndef STEWNODET_H
#define STEWNODET_H

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

#endif // STEWNODET_H
