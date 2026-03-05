#include "Buffer.h"
#include <string>

namespace yy
{

Buffer::Buffer(size_t initial_size,size_t prepend_size):
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
void Buffer::append(const char* data,size_t size)
{
    ensure_appendable(size);
    std::copy(data,data+size,begin_write());
    move_write_index(size);
}

void Buffer::append(const void* data,size_t size)
{
    append(static_cast<const char*>(data),size);
}
char* Buffer::append()// FIXMETH 不移动指针，可读可写指针向外暴露?
{
    return begin_write();
}
void Buffer::append(size_t size)
{
    move_write_index(size);
}
char* Buffer::retrieve(size_t size)
{
    if(size<=get_readable_size())
    {
        char* r=begin_read();
        move_read_index(size);
        return r;
    }
    return nullptr;
}
char* Buffer::retrieveAll()
{
    auto size=get_readable_size();
    return retrieve(size);
}

std::string Buffer::retrieveAllToString()
{
    
    return std::string(retrieveAll());
}    
void Buffer::ensureWritableBytes(size_t len)
{
    if (get_writable_size()<len)
    {
        expend(len);
    }
    assert(get_writable_size() >= len&&"可写尺寸小于需写尺寸");
    check_index_validity(__FILE__, __LINE__);
}
void Buffer::hasWritten(size_t len)
{
    assert(len<=get_writable_size());
    write_index_+=len;
    check_index_validity(__FILE__, __LINE__);
}
void Buffer::unwrite(size_t len)
{
    assert(len<=get_readable_size());
    write_index_-=len;
    check_index_validity(__FILE__, __LINE__);
}
void Buffer::shrink(size_t reserve)
{
    buffer_.resize(get_readable_size()+reserve+prepend_size_);
    buffer_.shrink_to_fit();
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
void Buffer::ensure_appendable(size_t size)
{
    if(get_writable_size()<size)
    {
        expend(size);
    }
    assert(get_writable_size()>=size&&"可写尺寸小于需写尺寸");
    check_index_validity(__FILE__,__LINE__);
}
void Buffer::move_read_index(size_t size)
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
void Buffer::move_write_index(size_t size)
{
    assert(size<=get_writable_size()&&"可写尺寸不够");
    write_index_+=size;
}
void Buffer::expend(size_t size)
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
    size_t readable=get_readable_size();
    std::copy(begin()+read_index_,
                begin()+write_index_,
                begin()+prepend_size_);
    read_index_=prepend_size_;
    write_index_=read_index_+readable;
    check_index_validity(__FILE__, __LINE__);        
}    

}