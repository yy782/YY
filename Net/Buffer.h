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
    explicit Buffer(byte_size initial_size=1024,byte_size prepend_size=8);
    void swap(Buffer& other);

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
    void check_index_validity(const char* file, int line)const;
    void ensure_appendable(byte_size size);
    void move_write_index(byte_size size)
    {
        assert(size<=get_writable_size()&&"可写尺寸不够");
        write_index_+=size;
    }
    void move_read_index(byte_size size);
    char* begin_write(){return begin()+write_index_;}
    char* begin(){return &*buffer_.begin();}
    const char* begin()const{return &*buffer_.begin();}
    void expend(byte_size size);
    void reuse_prependable_space();
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