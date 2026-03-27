#ifndef _YY_NET_TCP_BUFFER_H_
#define _YY_NET_TCP_BUFFER_H_

#include <vector>
#include <algorithm>
#include <string>
#include "noncopyable.h"
#include "../Common/stringPiece.h"
#include <cstring>
#include <assert.h>
namespace yy
{
namespace net
{



/**
 * @brief TCP缓冲区类，用于处理网络数据的读写
 * 
 * TcpBuffer是一个非循环缓冲区，用于处理TCP连接中的数据读写。
 * 它提供了一系列方法来管理缓冲区中的数据，包括添加数据、读取数据、查找边界等。
 */
class TcpBuffer:noncopyable
{
public: // @note 由于IO操作在Loop线程完成，保证了指针不会出乎意外的无效，但是将数据交给计算密集型线程池，需要拷贝数据

    /**
     * @brief 字符容器类型
     */
    typedef std::vector<char> CharContainer;

    /**
     * @brief 构造函数
     * 
     * @param initial_size 初始缓冲区大小
     * @param prepend_size 前置空间大小
     */
    explicit TcpBuffer(size_t initial_size=1024,size_t prepend_size=8);
    
    /**
     * @brief 析构函数
     */
    ~TcpBuffer()=default;
    
    /**
     * @brief 交换缓冲区
     * 
     * @param other 另一个缓冲区
     */
    void swap(TcpBuffer& other) noexcept;
    
    /**
     * @brief 添加数据
     * 
     * @tparam T 数据类型
     * @param value 要添加的数据
     */
    template<typename T>
    void append(T&& value);
    
    /**
     * @brief 添加数据
     * 
     * @param data 数据指针
     * @param size 数据大小
     */
    void append(const char* data,size_t size);
    
    /**
     * @brief 添加数据（禁用，需要指明长度）
     * 
     * @param data 数据指针
     */
    void append(const char*){assert(false&&"请指明长度");}
    
    /**
     * @brief 从文件描述符读取数据
     * 
     * @param fd 文件描述符
     * @return ssize_t 读取的字节数
     */
    ssize_t appendFormFd(int fd);
    
    /**
     * @brief 流式添加数据
     * 
     * @tparam T 数据类型
     * @param value 要添加的数据
     * @return TcpBuffer& 缓冲区引用
     */
    template<typename T>
    TcpBuffer& FluentAppend(T&& value);
    
    /**
     * @brief 流式添加数据
     * 
     * @param data 数据指针
     * @param size 数据大小
     * @return TcpBuffer& 缓冲区引用
     */
    TcpBuffer& FluentAppend(const char* data,size_t size);
    
    /**
     * @brief 流式添加数据（禁用，需要指明长度）
     * 
     * @param data 数据指针
     * @return TcpBuffer& 缓冲区引用
     */
    TcpBuffer& FluentAppend(const char*)
    {
        assert(false&&"请指明长度");
        return *this;
    };
    
    /**
     * @brief 消费数据
     * 
     * @param size 消费的数据大小
     */
    void consume(size_t size);
    
    /**
     * @brief 获取可写位置
     * 
     * @return char* 可写位置指针
     */
    char* BeginWrite();
    
    /**
     * @brief 读取数据
     * 
     * @param size 读取的数据大小
     * @return char* 数据指针
     */
    char* retrieve(size_t size);
    
    /**
     * @brief 读取所有数据
     * 
     * @return char* 数据指针
     */
    char* retrieveAll();
    
    /**
     * @brief 读取所有数据并转换为字符串
     * 
     * @return std::string 数据字符串
     */
    std::string retrieveAllToString();
    
    /**
     * @brief 查看数据
     * 
     * @return const char* 数据指针
     */
    const char* peek()const noexcept{return begin()+read_index_;}
    
    /**
     * @brief 获取可读视图
     * 
     * @return stringPiece 可读视图
     */
    stringPiece readView()const noexcept{return stringPiece(peek(),readable_size()+1);}
    
    /**
     * @brief 修改数据
     * 
     * @return char* 数据指针
     */
    char* ModifyData(){return begin()+read_index_;}

    /**
     * @brief 收缩缓冲区
     * 
     * @param reserve 保留的大小
     */
    void shrink(size_t reserve);
    
    /**
     * @brief 获取可读大小
     * 
     * @return size_t 可读大小
     */
    size_t readable_size()const noexcept{return write_index_-read_index_;}
    
    /**
     * @brief 获取可写大小
     * 
     * @return size_t 可写大小
     */
    size_t writable_size()const noexcept{return buffer_.size()-write_index_;}
    
    /**
     * @brief 获取前置空间大小
     * 
     * @return size_t 前置空间大小
     */
    size_t prependable_size()const noexcept{return read_index_;}

    /**
     * @brief 查找边界
     * 
     * @param border 边界字符串
     * @return char* 边界位置指针
     */
    char* findBorder(const char* border) noexcept
    {
        return std::search(begin_read(),begin_write(),border,border+strlen(border));
    }
    
    /**
     * @brief 查找边界
     * 
     * @param border 边界字符串
     * @param size 边界大小
     * @return char* 边界位置指针
     */
    char* findBorder(const char* border,size_t size) noexcept
    {
        return std::search(begin_read(),begin_write(),border,border+size);
    }
    
    /**
     * @brief 查找边界
     * 
     * @param border 边界字符串
     * @param size 边界大小
     * @param len 从开始到边界的长度
     * @return char* 边界位置指针
     */
    char* findBorder(const char* border,size_t size,size_t& len) noexcept
    {
        char* last=findBorder(border,size);
        len=last-begin_read();
        return last;
    }
    
    /**
     * @brief 前置添加数据
     * 
     * @param data 数据指针
     * @param size 数据大小
     */
    void prepend(const char* data,size_t size) noexcept
    {
        assert(size <= prependable_size());
        read_index_ -= size;
        std::copy(data, data + size, begin_read());
    }
    
    /**
     * @brief 前置添加数据
     * 
     * @param data 数据指针
     * @param size 数据大小
     */
    void prepend(const void* data,size_t size) noexcept
    {
        prepend(static_cast<const char*>(data), size);
    }

    /**
     * @brief 确保可写空间
     * 
     * @param len 需要的可写空间大小
     */
    void ensureWritableBytes(size_t len);
    
    /**
     * @brief 标记已写
     * 
     * @param len 已写的大小
     */
    void hasWritten(size_t len);
    
    /**
     * @brief 标记未写
     * 
     * @param len 未写的大小
     */
    void unwrite(size_t len);

    /**
     * @brief 清空缓冲区
     */
    void clear() noexcept
    {
        read_index_=8;
        write_index_=8;
    }
private:
    /**
     * @brief 移动写索引
     * 
     * @param size 移动的大小
     */
    void move_write_index(size_t size);
    
    /**
     * @brief 移动读索引
     * 
     * @param size 移动的大小
     */
    void move_read_index(size_t size);
    
    /**
     * @brief 检查索引有效性
     */
    void check_index_validity()const;
    
    /**
     * @brief 确保可追加空间
     * 
     * @param size 需要的空间大小
     */
    void ensure_appendable(size_t size);

    /**
     * @brief 获取写位置
     * 
     * @return char* 写位置指针
     */
    char* begin_write(){return begin()+write_index_;}
    
    /**
     * @brief 获取读位置
     * 
     * @return char* 读位置指针
     */
    char* begin_read(){return begin()+read_index_;}
    
    /**
     * @brief 获取缓冲区开始位置
     * 
     * @return char* 缓冲区开始位置指针
     */
    char* begin(){return &*buffer_.begin();}
    
    /**
     * @brief 获取缓冲区开始位置
     * 
     * @return const char* 缓冲区开始位置指针
     */
    const char* begin()const{return &*buffer_.begin();}
    
    /**
     * @brief 扩展缓冲区
     * 
     * @param size 扩展的大小
     */
    void expend(size_t size);
    
    /**
     * @brief 重用前置空间
     */
    void reuse_prependable_space();

    /**
     * @brief 添加数据实现
     * 
     * @param data 数据指针
     * @param size 数据大小
     */
    void appendImp(const char* data,size_t size);
    
    /**
     * @brief 缓冲区
     */
    CharContainer buffer_;
    
    /**
     * @brief 前置空间大小
     * 
     * 考虑到连续内存，不采用循环队列实现
     */
    const size_t prepend_size_;
    
    /**
     * @brief 读索引
     */
    size_t read_index_;
    
    /**
     * @brief 写索引
     */
    size_t write_index_;
    
    // @brief 选择索引和buffer_的begin()迭代器使用而不是简单的char*,或者可读，可写的迭代器，防止动态扩容的失效


};
/**
 * @brief 添加数据
 * 
 * @tparam T 数据类型
 * @param value 要添加的数据
 */
template<typename T>
void TcpBuffer::append(T&& value)
{
    using DecayedT = std::decay_t<T>;
    static_assert(!std::is_same_v<DecayedT,std::string>);
    if constexpr(std::is_same_v<DecayedT,stringPiece>)
    {
        appendImp(value.data(),value.size());
    }
    else
        appendImp(reinterpret_cast<const char*>(&value), sizeof(T));
    
}

/**
 * @brief 流式添加数据
 * 
 * @tparam T 数据类型
 * @param value 要添加的数据
 * @return TcpBuffer& 缓冲区引用
 */
template<typename T>
TcpBuffer& TcpBuffer::FluentAppend(T&& value)
{
    append(std::forward<T>(value));
    return *this;
}

}
}
#endif