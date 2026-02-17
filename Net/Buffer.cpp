#include "Buffer.h"

namespace yy
{
namespace net
{
Buffer::Buffer(byte_size initial_size,byte_size prepend_size):
buffer_(prepend_size+initial_size),
prepend_size_(prepend_size),
read_index_(prepend_size),
write_index_(prepend_size)
{}
void Buffer::swap(Buffer& other)
{
    buffer_.swap(other.buffer_);
    std::swap(read_index_,other.read_index_);
    std::swap(write_index_,other.write_index_);
}

void Buffer::check_index_validity(const char* file, int line)const
{
#ifndef NDEBUG        
        auto read_index=std::to_string(read_index_);
        auto prepend_size=std::to_string(prepend_size_);
        auto context=std::string(file)+":"+std::to_string(line);
        auto check_read_ptr=context+" 可读指针是 "+read_index+" 预留大小是 "+prepend_size;
        assert(read_index_>=prepend_size_&&check_read_ptr.c_str());
        
        auto write_index=std::to_string(write_index_);
        auto check_write_ptr=context+" 可写指针是 "+write_index+" 可读指针是 "+read_index;
        assert(write_index_>=read_index_&&check_write_ptr.c_str());
#endif    
}
void Buffer::ensure_appendable(byte_size size)
{
    if(get_writable_size()<size)
    {
        expend(size);
    }
    assert(get_writable_size()>=size&&"可写尺寸小于需写尺寸");
    check_index_validity(__FILE__,__LINE__);
}
void Buffer::move_read_index(byte_size size)
{
    assert(size<=get_readable_size());
    if(size<get_readable_size())
    {
        read_index_+=size;
        check_index_validity(__FILE__, __LINE__);            
    }
    else 
    {
        check_index_validity(__FILE__, __LINE__);
        read_index_=prepend_size_;
        write_index_=prepend_size_;
        check_index_validity(__FILE__, __LINE__);
    }
}
void Buffer::expend(byte_size size)
{
    check_index_validity(__FILE__, __LINE__);
    if(get_writable_size()+get_prependable_size()>size+prepend_size_)
    {
        //复用
        reuse_prependable_space();
    }
    else
    {
        reuse_prependable_space();
        // @brief muduo没有选择进行优化，但是提出了建议。我选择进行优化
        buffer_.resize(write_index_+size);
        check_index_validity(__FILE__, __LINE__);
    }
}
void Buffer::reuse_prependable_space()
{
    byte_size readable=get_readable_size();
    std::copy(begin()+read_index_,
                begin()+write_index_,
                begin()+prepend_size_);
    read_index_=prepend_size_;
    write_index_=read_index_+readable;
    check_index_validity(__FILE__, __LINE__);        
}    
}    
}