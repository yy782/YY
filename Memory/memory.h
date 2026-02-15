#ifndef _MEMORY_H_
#define _MEMORY_H_
#include <iostream>
#include <vector>
#include <limits>
#include <type_traits>
#include <cstddef>
#include <memory>
#include <cassert>
#include "../Common/noncopyable.h"
#define LOG_FILE
#define COUT
#include "../Common/Log.h"
#include "../Common/Types.h"

namespace yy{    

typedef size_t byte_size;
typedef size_t align_size;

constexpr char* calc_aligned_addr(align_size align,char* bottom,byte_size heap_size)
{
    if(bottom==nullptr)return nullptr;
    if((align&(align-1))!=0)return nullptr;
    if(heap_size==0)return nullptr;
    uintptr_t addr=reinterpret_cast<uintptr_t>(bottom);
    size_t offset=addr%align;
    char* aligned_ptr=bottom;
    aligned_ptr+=(align-offset);
    byte_size pad_bytes=aligned_ptr-bottom; 
    if(pad_bytes>=heap_size){
        return nullptr;
    }
    return aligned_ptr;
}

class MemoryPool:noncopyable {
public:
    


    static MemoryPool& get_instance(byte_size capacity=0);
    void set_capacity(byte_size capacity);
    ~MemoryPool();

    void* malloc(byte_size size);
    template<typename Function>
    void* malloc(byte_size size,Function&& f);
    void* malloc(byte_size size,align_size align);
    void free(void* ptr,byte_size size); 
       
private:
    MemoryPool();
    MemoryPool(byte_size capacity);


    static constexpr size_t BUCKET_SIZE=5;
    
    struct FreeBlock{
        
        char* ptr_;
        byte_size size_;
        
        std::shared_ptr<FreeBlock> next_;
        std::weak_ptr<FreeBlock> prev_;
        FreeBlock():
        ptr_(nullptr),
        size_(0),
        next_(nullptr),
        prev_()
        {}
        FreeBlock(char* ptr,byte_size size):
        ptr_(ptr),
        size_(size),
        next_(nullptr),
        prev_()
        {}
    };

    typedef std::shared_ptr<FreeBlock>  FreeBlock_Ptr;
    typedef std::weak_ptr<FreeBlock> FreeBlock_Weak_Ptr;

    struct Bucket:noncopyable{
        size_t block_size_;//桶固定块大小
        FreeBlock_Ptr free_list_;
        MemoryPool* pool_;
        Bucket()
        :block_size_(0),
        free_list_(nullptr),
        pool_(nullptr)
        {}
        Bucket(size_t block_size,MemoryPool* pool)
        :block_size_(block_size),
        free_list_(nullptr),
        pool_(pool)
        {}
        Bucket& operator=(Bucket&& other);
        Bucket(Bucket&& other)=delete;
        char* alloc(byte_size size,align_size align);
        void restore(char* ptr,byte_size size);
        ~Bucket();
    };
    void init_buckets();
    char* alloc_form_system(byte_size capacity);

    char* alloc_from_bucket(byte_size size,align_size align);
    void restore_bucket(char* ptr,byte_size size); //这里是将归还的堆内存交给桶管理；
    size_t match_bucket_index(byte_size size);
    //size_t match_bucket_index(byte_size size,align_size align);
    


    static constexpr size_t DEFAULT_ALIGN=alignof(std::max_align_t);//默认对齐值是最大对齐值
    Bucket buckets_[BUCKET_SIZE];
    byte_size heap_capacity;
    std::vector<void*> bass_address;
    char* bottom;
    char* top;
    
    friend struct Bucket;
};

template<typename Function>
void* MemoryPool::malloc(byte_size size,Function&& f)
{
    constexpr align_size align=alignof(Function);
    return malloc(size,align);
}




class HeapMemoryManager{//堆内存管理器,线程安全,容器库通过allocate分配内存后要到这里注册，这样容器不用注意堆内存的安全，要求对堆内存的操作通过HeapMemoryManager进行

};
}


#endif