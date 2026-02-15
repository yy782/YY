#include "memory.h"

namespace yy 
{
MemoryPool& MemoryPool::get_instance(byte_size capacity)
{
    static auto cap=capacity;
    if(cap == 0)
    { 
        static MemoryPool pool;
        return pool; 
    }
    static MemoryPool pool(cap);
    return pool;
}   

void MemoryPool::set_capacity(byte_size capacity)
{
 heap_capacity = capacity;
 bottom=static_cast<char*>(alloc_form_system(capacity));
 if(bottom == nullptr)
 {
     LOG_SYSTEM_ERROR("[MemoryPool::set_capacity] alloc_form_system failed");
     return;
 }
 top=bottom+capacity;
 init_buckets();
}
MemoryPool::MemoryPool()
:heap_capacity(0),
bottom(NULL), 
top(NULL)
{
    init_buckets();
}

MemoryPool::MemoryPool(byte_size capacity):
heap_capacity(capacity),
bottom(static_cast<char*>(alloc_form_system(capacity))),
top(bottom+heap_capacity)
{
    init_buckets();
}
void* MemoryPool::malloc(byte_size size){
    
    return malloc(size,DEFAULT_ALIGN);
}



void* MemoryPool::malloc(byte_size size,align_size align)
{
    if(size==0||(align&(align-1))!=0)return nullptr;
    char* ptr=alloc_from_bucket(size,align);
    if(ptr!=NULL){
        return ptr;
    }else if(safe_static_cast<byte_size>(bottom-top)>=size){
        ptr=calc_aligned_addr(align,bottom,top-bottom);
        auto aligned_size=ptr-bottom;
        restore_bucket(bottom,aligned_size);
        bottom=ptr+size;
        LOG_MEMORY_DEBUG("[MemoryPool] 分配了"<<size<<"字节内存");      
        return ptr;
    }else{
//这里我想向系统归还未使用的top到bottom的内存，但是标准库不允许，只能要求归还完整的堆内存,所以我将堆内存交给桶管理，在申请更大的内存。
        LOG_MEMORY_INFO("[MemoryPool] 尝试向系统申请内存");
        restore_bucket(top,top-bottom);
        bottom=static_cast<char*>(alloc_form_system(heap_capacity));
        top=bottom+heap_capacity;
        return alloc_form_system(size);
    }     
}

void MemoryPool::free(void* ptr,byte_size size)
{
    if(ptr==nullptr||size==0)return;
    LOG_MEMORY_DEBUG("[MemoryPool] 内存归还"<<size<<"字节");
    restore_bucket(static_cast<char*>(ptr),size);
}

void MemoryPool::init_buckets(){
    constexpr size_t bucket_specs[]={64,128,256,512,std::numeric_limits<size_t>::max()};
    for(size_t i=0;i<BUCKET_SIZE;i++)
    {
        buckets_[i]=Bucket(bucket_specs[i],this);
    }    
}
char* MemoryPool::alloc_form_system(byte_size capacity)
{
    void* ptr=::operator new(capacity,std::nothrow);//不抛出异常，但是返回空指针
    if(ptr!=nullptr)
    {
     LOG_MEMORY_DEBUG("[MemoryPool::alloc_form_system] allocate memory from system,size:"<<capacity);   
     bass_address.push_back(ptr);   
    }
    
    return static_cast<char*>(ptr);
}


char* MemoryPool::alloc_from_bucket(byte_size size,align_size align)
{
    if(size==0||(align&(align-1))!=0)return nullptr;
    size_t index=match_bucket_index(size+DEFAULT_ALIGN);
    if(index<BUCKET_SIZE)
    {
        return buckets_[index].alloc(size,align);
    }
    return nullptr;
}
void MemoryPool::restore_bucket(char* ptr,byte_size size)
{
    if(size==0||ptr==nullptr)return;
    size_t index=match_bucket_index(size);
    assert(index<BUCKET_SIZE&&"index out of range");
    return buckets_[index].restore(ptr,size);
}



size_t MemoryPool::match_bucket_index(byte_size size)
{
    if(size==0)return BUCKET_SIZE;
    for(byte_size i=0;i<BUCKET_SIZE;i++)
    {
        if(buckets_[i].block_size_>=size)
        {
            return i;
        }
    }
    return BUCKET_SIZE;
}

MemoryPool::~MemoryPool()
{
    for(auto iter=bass_address.begin();iter!=bass_address.end();iter++)
    {
        assert(*iter!=nullptr&&"申请的首地址为空");
        ::operator delete(*iter);

    }
}
 MemoryPool::Bucket& MemoryPool::Bucket::operator=(Bucket&& other)
 {
        block_size_=other.block_size_;
        free_list_=other.free_list_;
        pool_=other.pool_;

        other.free_list_=nullptr;
        other.pool_=nullptr;
        return *this;
 }

char* MemoryPool::Bucket::alloc(byte_size size,align_size align)
{ 
    assert(size>0&&((align&(align-1))==0)&&"size必须大于0，并且对齐值必须是2的幂");
    FreeBlock_Ptr cur=free_list_;
    while(cur)
    {
        if(cur->size_<size)
        {
            cur=cur->next_;
            continue;
        }
        auto start_ptr=cur->ptr_;
        char* aligned_ptr=calc_aligned_addr(align,start_ptr,cur->size_);
        if(aligned_ptr==nullptr)
        {
            cur=cur->next_;
            continue;
        }
        auto aligned_size=aligned_ptr-start_ptr;
        auto remain_size=cur->size_-aligned_size;
        if(remain_size<size)
        {
            cur=cur->next_;
            continue;
        }//这里确定可以划分内存出去，接下来要更新节点数据
        if(aligned_size>0)
        {
            assert(pool_!=nullptr&&"内存池指针为空");
            pool_->restore_bucket(start_ptr,aligned_size);
        }
        auto node_remain_size=cur->size_-aligned_size-size;
        if(node_remain_size>0)
        {
            cur->ptr_=aligned_ptr+size;
            cur->size_=node_remain_size;
        }else{
            auto prev_node=cur->prev_.lock();
            auto next_node=cur->next_;
            if(prev_node)
            {
                prev_node->next_=next_node;
            }else{
                free_list_=next_node;
            }
            if(next_node)
            {
                next_node->prev_=prev_node;
            }
            cur->next_=nullptr;
        }
        return aligned_ptr;
    }
    return nullptr;
}

void MemoryPool::Bucket::restore(char* ptr,byte_size size)
{
    auto node_ptr=std::make_shared<FreeBlock>(ptr,size);
    if(free_list_==nullptr)
    {
        free_list_=node_ptr;
        return;
    }
    node_ptr->next_=free_list_->next_;
    node_ptr->prev_=free_list_;
    free_list_=node_ptr;
}

MemoryPool::Bucket::~Bucket()
{
    free_list_.reset();//这里节点管理的内存由内存池统一释放
}     
}
