#ifndef _YY_MEMORYPOOL_H_
#define _YY_MEMORYPOOL_H_

#include <cstddef>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <cstdlib>



namespace yy
{



/** 
 * @param
 *  MEMORYPOOL_NUM 内存池数量，分为8，64，.... 
 *  SLOT_BASE_SIZE 内存池的基础槽大小为(index+1)*8
 *  MAX_SLOT_SIZE 最大槽大小
*/    
constexpr size_t MEMORYPOOL_NUM=64; 
constexpr size_t SLOT_BASE_SIZE=8;  
constexpr size_t MAX_SLOT_SIZE=512; 


/**
 * @brief 槽结构，用于管理空闲内存块
 * 
 * 空闲内存槽通过next指针构成单链表，实现空闲内存的复用
 * 当内存被分配出去后，其内存空间不再保持Slot结构
 */
struct Slot
{
    Slot *next;
};

class MemoryPool
{
public:
    // BlockSize默认设计为4096字节是因为作系统页大小通常是 4KB，与页对齐可以提高内存访问效率，避免跨页访问导致的性能损失
    MemoryPool(size_t blockSize=4096);
    ~MemoryPool();

    void init(size_t slotSize); // 延迟初始化
    void *allocate();           // 分配一个内存槽，返回槽指针
    void deallocate(void *p);   // 回收内存槽到空闲槽链表
private:
    void allocateNewBlock();                     // 向OS申请一个新内存块
    size_t padPointer(char *p, size_t slotSize); // 计算内存对齐

    size_t blockSize_; 
    size_t slotSize_;  

    Slot *firstBlock_; 
    Slot *curSlot_;   
    Slot *lastSlot_;
    Slot *freeList_;   
       

/**
 * @param
 * - firstBlock_: 所有内存块构成的链表，便于统一释放
 * - curSlot_: 当前内存块中下一个可用槽的指针
 * - lastSlot_: 当前内存块的边界指针，超过此值表示需要新内存块
 * - freeList_: 回收的空闲槽链表，实现快速复用
 */

    std::mutex mutexForFreeList_; // 保证freeList_在多线程中操作的原子性
    std::mutex mutexForBlock_;    // 保证内存块管理在多线程中操作的原子性
};

/**
 * @brief 哈希桶类，管理多个不同槽大小的内存池
 * 
 * 采用单例模式，维护64个内存池，分别管理8B~512B（步长8B）的内存槽
 * 提供统一的内存分配/释放接口，根据请求大小路由到对应的内存池或直接使用malloc/free
 */
class PoolBucket
{
public:
    static void initMemoryPool();
    static MemoryPool &getMemoryPool(size_t index);
    static void* malloc(size_t size);
    static void free(void *p, size_t size);
private:
    static MemoryPool memoryPool[MEMORYPOOL_NUM]; // 设置为static，借助 C++11 的线程安全静态初始化保证只初始化一次
};

template <typename T,typename... Args>
T* newElement(Args &&...args)
{
    T *res=static_cast<T*>(PoolBucket::malloc(sizeof(T)));
    if(res)
    {
        new (res) T(std::forward<Args>(args)...);
    }
    return res;
}

template <typename T>
void deleteElement(T *p)
{
    if (p)
    {
        p->~T();
        PoolBucket::free(reinterpret_cast<void *>(p), sizeof(T));
    }
}

}
#endif