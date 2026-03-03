#ifndef _YY_SHAREDSTRINGSTRING_H_
#define _YY_SHAREDSTRINGSTRING_H_
#include "copyable.h"
#include <atomic>
#include <cstddef>
#include <malloc.h>
#include <assert.h>
#include <cstring>
namespace yy
{
class SharedString : copyable
{
public:
    SharedString()=default;
    SharedString(const char* data,size_t len): 
    capacity_(len*2),
    ptr_(new char[capacity_]),
    size_(len),
    count_(new std::atomic<size_t>(1))
    {
        assert(ptr_);
        memcpy(ptr_,data,len);
        ptr_[len]='\0'; 
    }
    
    SharedString(const SharedString& other):  
    capacity_(other.capacity_),
    ptr_(other.ptr_),
    size_(other.size_),
    count_(other.count_)
    {
        ++(*count_);
    }
    SharedString& operator=(const SharedString& other)
    {
        if (this!=&other) {
            if(--(*count_)==0) 
            {
                delete[] ptr_;
            }
            capacity_=other.capacity_;
            ptr_=other.ptr_;
            size_=other.size_;
            count_=other.count_;
            ++(*count_);
        }
        return *this;
    }
    
    ~SharedString()
    {
        if(--(*count_)==0)
        {
            delete[] ptr_;
        }
    }
    const char* c_str()const{ return ptr_;}
    size_t size()const{ return size_;}
    
    bool empty()const{return size_ == 0;}
    
    size_t use_count()const{return (*count_).load();}
    
    char operator[](size_t index)const 
    { 
        assert(index < size_);
        return ptr_[index]; 
    }
    
    bool operator==(const char* s) const 
    { 
        assert(s);
        return strcmp(ptr_,s)==0; 
    } 
    bool operator==(const SharedString& other)const 
    { 
        if(size_!=other.size_)return false;
        return memcmp(ptr_,other.ptr_,size_)==0;
    }
    
private:
    ssize_t capacity_=0;
    char* ptr_=nullptr;
    size_t size_=0; 
    mutable std::atomic<size_t>* count_{0};
};
}

#endif