#include <vector>
#include <algorithm>
#include <string>
#include "noncopyable.h"
#include <cstring>

namespace yy
{
namespace net
{
// @brief 缓冲区只管存，不管该存多大,存多大由业务判断
class TcpBuffer:noncopyable
{
public: // @note 由于IO操作在Loop线程完成，保证了指针不会出乎意外的无效，但是将数据交给计算密集型线程池，需要拷贝数据

    typedef std::vector<char> CharContainer;

    explicit TcpBuffer(size_t initial_size=1024,size_t prepend_size=8);
    ~TcpBuffer()=default;
    void swap(TcpBuffer& other);

    void append(const char* data,size_t size);
    void append(const void* data,size_t size);
    char* append();
    void append(size_t size);
    char* retrieve(size_t size);
    char* retrieveAll();
    
    std::string retrieveAllToString();
    const char* peek()const {return begin()+read_index_;}
    char* peek(){return begin()+read_index_;}
    void shrink(size_t reserve);
    size_t get_readable_size()const{return write_index_-read_index_;}
    size_t get_writable_size()const{return buffer_.size()-write_index_;}
    size_t get_prependable_size()const{return read_index_;}

    char* findBorder(const char* border) 
    {
        return std::search(begin_read(),begin_write(),border,border+strlen(border));
    }
    char* findBorder(const char* border,size_t size)
    {
        return std::search(begin_read(),begin_write(),border,border+size);
    }
    char* findBorder(const char* border,size_t size,size_t& len) // FIXME
    {
        char* last=findBorder(border,size);
        len=last-begin_read();
        return last;
    }
    void ensureWritableBytes(size_t len);
    void hasWritten(size_t len);
    void unwrite(size_t len);


        /**
     * @brief 背压配置结构体
     * 
     * 包含背压机制的所有可调参数，用于精细控制单个连接的流量行为。
     */


private:
    void move_write_index(size_t size);
    void move_read_index(size_t size);
    void check_index_validity(const char* file, int line)const;
    void ensure_appendable(size_t size);

    char* begin_write(){return begin()+write_index_;}
    char* begin_read(){return begin()+read_index_;}
    char* begin(){return &*buffer_.begin();}
    const char* begin()const{return &*buffer_.begin();}
    void expend(size_t size);
    void reuse_prependable_space();

    

    CharContainer buffer_;
    // @brief 考虑到连续内存，不采用循环队列实现
    const size_t prepend_size_;
    size_t read_index_;
    size_t write_index_;
    // @brief 选择索引和buffer_的begin()迭代器使用而不是简单的char*,或者可读，可写的迭代器，防止动态扩容的失效

}; 

}
}