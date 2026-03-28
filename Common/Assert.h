#include <atomic>
#include <sys/mman.h>

std::atomic<int> Num;

void add()
{
    ++Num;
}

bool isPointerValid(void* ptr, size_t size) {
    // msync 会检查内存区域是否有效
    return msync(ptr, size, MS_ASYNC) == 0;
}
