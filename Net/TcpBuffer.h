#ifndef _YY_NET_TCP_BUFFER_H_
#define _YY_NET_TCP_BUFFER_H_

#include <vector>
#include <algorithm>
#include <string>
#include "noncopyable.h"
#include "string_view.h"
#include <cstring>
#include <assert.h>
namespace yy
{
namespace net
{



// @brief 缓冲区只管存，不管该存多大
class TcpBuffer:noncopyable
{
public: // @note 由于IO操作在Loop线程完成，保证了指针不会出乎意外的无效，但是将数据交给计算密集型线程池，需要拷贝数据

    typedef std::vector<char> CharContainer;

    enum class ElementType
    {
        Uint32,
        Uint64, 
        CharPtr,
        ConstCharPtr,
        StringView,
        Void,
        TcpBufferReference
    };
    template<ElementType T>
    struct ElementTypeTraits
    {
        using type="Unknown ElementType";
    };

    explicit TcpBuffer(size_t initial_size=1024,size_t prepend_size=8);
    ~TcpBuffer()=default;
    void swap(TcpBuffer& other);
    template<ElementType T,ElementType U=ElementType::Void>
    auto append(typename ElementTypeTraits<T>::type value)->typename ElementTypeTraits<U>::type;
    void append(const char* data,size_t size);

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
        return std::search(BeginRead(),BeginWrite(),border,border+strlen(border));
    }
    char* findBorder(const char* border,size_t size)
    {
        return std::search(BeginRead(),BeginWrite(),border,border+size);
    }
    char* findBorder(const char* border,size_t size,size_t& len)
    {
        char* last=findBorder(border,size);
        len=last-BeginRead();
        return last;
    }
    
    
    void prepend(const char* data,size_t size)
    {
        assert(size <= get_prependable_size());
        read_index_ -= size;
        std::copy(data, data + size, BeginRead());
    }
    
    void prepend(const void* data,size_t size)
    {
        prepend(static_cast<const char*>(data), size);
    }
    
    void ensureWritableBytes(size_t len);
    void hasWritten(size_t len);
    void unwrite(size_t len);

    char* BeginWrite(){return begin()+write_index_;}
    char* BeginRead(){return begin()+read_index_;}    

private:
    void move_write_index(size_t size);
    void move_read_index(size_t size);
    void check_index_validity(const char* file, int line)const;
    void ensure_appendable(size_t size);


    char* begin(){return &*buffer_.begin();}
    const char* begin()const{return &*buffer_.begin();}
    void expend(size_t size);
    void reuse_prependable_space();

    void appendImp(const char* data,size_t size);

    CharContainer buffer_;
    // @brief 考虑到连续内存，不采用循环队列实现
    const size_t prepend_size_;
    size_t read_index_;
    size_t write_index_;
    // @brief 选择索引和buffer_的begin()迭代器使用而不是简单的char*,或者可读，可写的迭代器，防止动态扩容的失效

}; 




template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::Uint32> {
    using type=uint32_t;
};

template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::StringView> {
    using type=string_view;
};

template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::CharPtr> {
    using type=char*;
};
template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::ConstCharPtr> {
    using type=const char*;
};

template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::Void> {
    using type=void;
};
template<>
struct TcpBuffer::ElementTypeTraits<TcpBuffer::ElementType::TcpBufferReference> {
    using type=TcpBuffer&;
};

template<TcpBuffer::ElementType T,TcpBuffer::ElementType U>
auto TcpBuffer::append(typename ElementTypeTraits<T>::type value)->typename ElementTypeTraits<U>::type
{
    using ValueType=typename ElementTypeTraits<T>::type;
    using ReturnType=typename ElementTypeTraits<U>::type;
    if constexpr (ValueType==uint32_t)
    {
        append(reinterpret_cast<const char*>(&value),sizeof(ValueType));
    }
    else if constexpr (ValueType==string_view)
    {
        append(value.data(),value.size());
    }
    else if constexpr (ValueType==char*)
    {
        append(value,strlen(value));
    }
    else if constexpr (ValueType==const char*)
    {
        append(value,strlen(value));
    }
    else
    {
        static_assert("Unsupported ValueType");
    }

    if constexpr (ReturnType==void)
    {
        return;
    }
    else if constexpr (ReturnType==TcpBuffer&)
    {
        return *this;
    }
    else if constexpr (ReturnType==char*)
    {
        return BeginWrite();
    }

    return static_assert("Unsupported ReturnType");

}

}
}
#endif