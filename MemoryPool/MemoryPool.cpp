#include "MemoryPool.h"

#include <assert.h>
namespace yy
{
/**
 * @param
 * - firstBlock_: 所有内存块构成的链表，便于统一释放
 * - curSlot_: 当前内存块中下一个可用槽的指针
 * - lastSlot_: 当前内存块的边界指针，超过此值表示需要新内存块
 * - freeList_: 回收的空闲槽链表，实现快速复用
 */
MemoryPool PoolBucket::memoryPool[MEMORYPOOL_NUM];

MemoryPool::MemoryPool(size_t blockSize): 
    blockSize_(blockSize),
    slotSize_(0),
    firstBlock_(nullptr),
    curSlot_(nullptr),
    lastSlot_(nullptr),
    freeList_(nullptr)
    {

    }

MemoryPool::~MemoryPool() 
{
    Slot* cur=firstBlock_;
    while(cur) 
    {
        Slot* next=cur->next;  // 保存下一个内存块指针
        ::free(cur);  
        cur=next;
    }
}

void MemoryPool::init(size_t slotSize)
{
    slotSize_=slotSize;
    allocateNewBlock();
}

/**
 * @brief 分配一个内存槽
 * 
 * 分配策略：
 * 1. 优先从空闲链表(freeList_)中分配（已回收的槽）
 * 2. 如果空闲链表为空，从当前内存块中分配新槽
 * 3. 如果当前内存块已用完，申请新内存块
 * 
 * @return void* 分配的内存地址
 */
void* MemoryPool::allocate() 
{
    if(freeList_) 
    {
        std::lock_guard<std::mutex> lock(mutexForFreeList_);
        if(freeList_) 
        {
            Slot* slot=freeList_;
            freeList_=freeList_->next;
            return slot;
        }
    }
    Slot* temp=nullptr;
    {
        std::lock_guard<std::mutex> lock(mutexForBlock_);
        if(curSlot_>lastSlot_) 
        {
            // 说明此时该内存块中也没有可用的内存槽，需要申请新的内存块
            // 申请新内存块后，curSlot_与lastSlot_会更新为指向新内存块
            allocateNewBlock();
        }
        temp=curSlot_;
        curSlot_+=slotSize_/sizeof(Slot);
    }
    return temp;
}

/**
 * @brief 回收内存槽
 * 
 * 使用头插法将回收的槽插入空闲链表头部
 * 这样下次分配时可以快速获取最近释放的内存（提高缓存命中率）
 * 
 * @param p 要回收的内存指针
 */
void MemoryPool::deallocate(void* p) 
{
    if(p) 
    {
        std::lock_guard<std::mutex> lock(mutexForFreeList_);
        Slot* slot=reinterpret_cast<Slot*>(p);
        slot->next=freeList_;
        freeList_=slot;
    }
}
/**
 * @brief 申请新的内存块
 * 
 * 分配一个新内存块，并初始化内部指针：
 * 1. 使用malloc分配blockSize_大小的内存
 * 2. 将新块插入内存块链表头部（firstBlock_）
 * 3. 计算对齐后的起始地址作为curSlot_
 * 4. 计算块的结束边界作为lastSlot_
 */
void MemoryPool::allocateNewBlock() 
{
    void* newBlock=::malloc(blockSize_);
    if(newBlock) 
    {
        reinterpret_cast<Slot*>(newBlock)->next=firstBlock_;
        firstBlock_=reinterpret_cast<Slot*>(newBlock);
    }
    char* endofhead=reinterpret_cast<char*>(newBlock)+sizeof(Slot*);
    size_t paddingsize=padPointer(endofhead,slotSize_);
    curSlot_=reinterpret_cast<Slot*>(endofhead+paddingsize);

    lastSlot_=reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock)+
                                        blockSize_-slotSize_+1);
    freeList_=nullptr;
}

size_t MemoryPool::padPointer(char* p, size_t slotSize) 
{
    return (slotSize-reinterpret_cast<size_t>(p))%slotSize;
}

void PoolBucket::initMemoryPool() 
{
    for(size_t i=0;i<MEMORYPOOL_NUM;++i) 
    {
        MemoryPool& pool=getMemoryPool(i);
        pool.init((i+1)*SLOT_BASE_SIZE);
        
    }
}

MemoryPool& PoolBucket::getMemoryPool(size_t index){return memoryPool[index]; }

void* PoolBucket::malloc(size_t size) 
{
    assert(size>0); // malloc作为内部接口，检验size是否非法是newElement负责检测
    if(size>MAX_SLOT_SIZE) 
    {
        return ::malloc(size);
    }

    size_t index=(size+7)/SLOT_BASE_SIZE-1;
    return getMemoryPool(index).allocate();
}

void PoolBucket::free(void* p, size_t size) 
{
    assert(p&&size>0);

    if(size>MAX_SLOT_SIZE) 
    {
        ::free(p);
        return;
    }

    size_t index=(size+7)/SLOT_BASE_SIZE-1;
    getMemoryPool(index).deallocate(p);
    return;
}
}