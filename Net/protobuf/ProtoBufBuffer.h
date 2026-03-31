#ifndef _YY_NET_BUFFEROUTPUTSTREAM_H_
#define _YY_NET_BUFFEROUTPUTSTREAM_H_


#include <google/protobuf/io/zero_copy_stream.h>
#include "../include/TcpBuffer.h"
namespace yy
{
namespace net
{

    

// ProtoBufOutputStream.h
class ProtoBufOutputStream : public google::protobuf::io::ZeroCopyOutputStream 
{
public:
    typedef TcpBuffer Buffer;
    ProtoBufOutputStream(Buffer* buf): 
    buffer_(buf),
    originalSize_(buffer_->readable_size()),
    byte_count_(0)
    {}

    // Next: 获取下一块可写内存
    bool Next(void** data, int* size) override 
    {
        // 每次至少分配 4KB，但也要考虑当前可写空间
        size_t free_space = buffer_->writable_size();
        if (free_space < 4096) {
            buffer_->ensureWritableBytes(4096);
        }
        *data = buffer_->BeginWrite();
        *size = static_cast<int>(buffer_->writable_size());
        
        // 标记这些空间已被使用
        buffer_->hasWritten(*size);
        byte_count_ += *size;
        
        return true;
    }

    // BackUp: 回退未使用的字节
    void BackUp(int count) override 
    {
        if (count > 0) {
            buffer_->unwrite(count);
            byte_count_ -= count;
        }
    }

    // ByteCount: 返回已写入的字节数
    int64_t ByteCount() const override 
    {
        return byte_count_;
    }

    // 获取实际写入的消息长度（不包括之前占位的长度字段）
    size_t getMessageSize() const 
    {
        return byte_count_;
    }

private:
    Buffer* buffer_;
    size_t originalSize_;  // 记录原始可读位置
    int64_t byte_count_;   // 记录已写入字节数
};

// ProtoBufInputStream.h
class ProtoBufInputStream : public google::protobuf::io::ZeroCopyInputStream 
{
public:
    ProtoBufInputStream(Buffer* buffer) : 
    buffer_(buffer), 
    position_(0),
    total_bytes_(buffer_->readable_size())
    {}

    bool Next(const void** data, int* size) override 
    {
        if (position_>=total_bytes_) 
        {
            return false;
        }
        
        // 返回当前可读数据的指针（从 peek() 开始 + 偏移）
        *data=buffer_->peek()+position_;
        
        // 剩余所有可读数据
        *size = static_cast<int>(total_bytes_-position_);
        
        position_+=*size;
        
        return true;
    }

    void BackUp(int count) override 
    {
        if (count > 0) 
        {
            position_=(static_cast<size_t>(count)>position_)?0:position_-static_cast<size_t>(count);
        }
    }

    bool Skip(int count) override 
    {
        if (count<0||position_+static_cast<size_t>(count)>total_bytes_) 
        {
            return false;
        }
        position_+=static_cast<size_t>(count);
        return true;
    }

    int64_t ByteCount() const override 
    {
        return position_;
    }

    // 消费已解析的数据
    void consume() 
    {
        buffer_->consume(position_);
    }

private:
    Buffer* buffer_;
    size_t position_;
    size_t total_bytes_;
};
}
}
#endif