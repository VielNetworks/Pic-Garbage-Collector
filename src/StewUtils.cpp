#include "StewUtils.h"
#include <chrono>
#include <thread>

use namespace StewGC;

//********************************************************************************

void Utils::ThreadSpin()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds(100);
    do std::this_thread::yield(); while (std::chrono::high_resolution_clock::now() < end);
}

//********************************************************************************
