#include <atomic>


std::atomic<int> Num;


void add()
{
    ++Num;
}