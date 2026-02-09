#include "../memory.h"
using namespace yy;

struct A
{
    int a;
    int b;
    char c;
    char* d;
    A(){}
    A(int a,int b,char c,char* d):a(a),b(b),c(c),d(d){}
};
int main()
{
    init_log_file();
    set_log_global_level(LOG_LEVEL_DEBUG);
    enable_log_module(LogModule::MEMORY);
    auto& memory_pool =MemoryPool::get_instance(1200);
    void* ptr = memory_pool.malloc(sizeof(A));
    A* obj=new (ptr) A(1,2,'c',nullptr);
    void* ptr2 = memory_pool.malloc(400);
    memory_pool.free(ptr,100);

    void* ptr3 = memory_pool.malloc(500);
    void* ptr4 = memory_pool.malloc(600);
    void* ptr5 = memory_pool.malloc(2);
    void* ptr6 = memory_pool.malloc(3);
    void* ptr7 = memory_pool.malloc(4);

    memory_pool.free(ptr5,2);
    memory_pool.free(ptr6,3);
    void* ptr8 = memory_pool.malloc(2);
    memory_pool.free(ptr7,4);
    void* ptr9 = memory_pool.malloc(3);
    memory_pool.free(ptr8,2);
    memory_pool.free(ptr9,3);
    
    
    void* ptr10 = memory_pool.malloc(sizeof(A),A());


    memory_pool.free(ptr2,400);
    memory_pool.free(ptr3,500);
    memory_pool.free(ptr4,600);
    memory_pool.free(obj,sizeof(A));
    close_log_file();
    return 0;
}