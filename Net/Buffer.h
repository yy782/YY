#ifndef _YY_NET_BUFFER_
#define _YY_NET_BUFFER_
#include <vector>
#include <assert.h>
#include "../Common/Errno.h"                
#include "../Common/Log.h"
#include "../Common/Types.h"
namespace yy
{
namespace net
{

 

// @brief 缓冲区只管存，不管该存多大,存多大由业务判断
class Buffer
{
public: 
    explicit Buffer(byte_size initial_size=1024,byte_size prepend_size=8):
    buffer_(prepend_size+initial_size),
    prepend_size_(prepend_size),
    read_index_(prepend_size),
    write_index_(prepend_size)
    {}
    void swap(Buffer& other)
    {
        buffer_.swap(other.buffer_);
        std::swap(read_index_,other.read_index_);
        std::swap(write_index_,other.write_index_);
    }
    //const std::vector<char>& get_buffer()const{return buffer_;}
    //std::vector<char>& get_buffer(){return buffer_;}

    void append(const char* data,byte_size size)
    {
        ensure_appendable(size);
        std::copy(data,data+size,begin_write());
        move_write_index(size);
    }

    void append(const void* data,byte_size size)
    {
        append(safe_static_cast<const char*>(data),size);
    }
    void retrieve(size_t size)
    {
        if(size<=get_readable_size())
        {
            move_read_index(size);
        }
    }
    const char* peek(){return begin()+read_index_;}
    void shrink(byte_size reserve)
    {
        buffer_.resize(get_readable_size()+reserve+prepend_size_);
        buffer_.shrink_to_fit();
    }
    byte_size get_readable_size()const{return write_index_-read_index_;}
    byte_size get_writable_size()const{return buffer_.size()-write_index_;}
    byte_size get_prependable_size()const{return read_index_;}
private:
    void check_index_validity(const char* file, int line)const
    {
#ifdef DEBUG        
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
    void ensure_appendable(byte_size size)
    {
        if(get_writable_size()<size)
        {
            expend(size);
        }
        assert(get_writable_size()>=size&&"可写尺寸小于需写尺寸");
        check_index_validity(__FILE__,__LINE__);
    }
    void move_write_index(byte_size size)
    {
        assert(size<=get_writable_size()&&"可写尺寸不够");
        write_index_+=size;
    }
    void move_read_index(byte_size size)
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
    char* begin_write(){return begin()+write_index_;}
    char* begin(){return &*buffer_.begin();}
    const char* begin()const{return &*buffer_.begin();}
    void expend(byte_size size)
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
    void reuse_prependable_space()
    {
        byte_size readable=get_readable_size();
        std::copy(begin()+read_index_,
                    begin()+write_index_,
                    begin()+prepend_size_);
        read_index_=prepend_size_;
        write_index_=read_index_+readable;
        check_index_validity(__FILE__, __LINE__);        
    }
    std::vector<char> buffer_;
    // @brief 考虑到连续内存，不采用循环队列实现
    const byte_size prepend_size_;
    size_t read_index_;
    size_t write_index_;
    // @brief 选择索引和buffer_的begin()迭代器使用而不是简单的char*,或者可读，可写的迭代器，防止动态扩容的失效
};    
}    
}
#endif