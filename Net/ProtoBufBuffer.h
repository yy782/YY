#ifndef _YY_NET_BUFFEROUTPUTSTREAM_H_
#define _YY_NET_BUFFEROUTPUTSTREAM_H_


#include <google/protobuf/io/zero_copy_stream.h>
#include "../Common/Buffer.h"
namespace yy
{
namespace net
{
class ProtoBufOutputStream:public google::protobuf::io::ZeroCopyOutputStream
{
public:
    ProtoBufOutputStream(Buffer* buf): 
    buffer_(buf),
    originalSize_(buffer_->get_readable_size())
    {}

    bool Next(void** data, int* size)override
    {
        buffer_->ensureWritableBytes(4096);
        *data = buffer_->append();
        *size = static_cast<int>(buffer_->get_writable_size());
        buffer_->hasWritten(*size);
        return true;
    }

    void BackUp(int count)override
    {
        buffer_->unwrite(count);
    }

    int64_t ByteCount()const override
    {
        return buffer_->get_readable_size() - originalSize_;
    }

private:
    Buffer* buffer_;
    size_t originalSize_;
};


class ProtoBufInputStream : public google::protobuf::io::ZeroCopyInputStream {
public:
    ProtoBufInputStream(Buffer* buffer) 
        : buffer_(buffer), 
          position_(0), 
          total_bytes_(buffer_->get_readable_size())  // 记录总可读字节数
    {}
    
    // 1. 获取下一块数据（必须实现）
    bool Next(const void** data, int* size) override {
        if (position_ >= total_bytes_) {
            return false;  // 没有更多数据
        }
        
        // 返回当前可读数据的指针
        *data = buffer_->peek() + position_;
        
        // 计算剩余可读字节数
        *size = static_cast<int>(total_bytes_ - position_);
        
        // 更新位置
        position_ += *size;
        
        return true;
    }
    
    // 2. 回退未使用的字节（必须实现）
    void BackUp(int count) override {
        if (count > 0) {
            // 确保不会回退超过已读取的位置
            if (count <= position_) {
                position_ -= count;
            } else {
                position_ = 0;
            }
        }
    }
    
    // 3. 跳过指定字节（必须实现）
    bool Skip(int count) override {
        if (count < 0) {
            return false;
        }
        
        // 检查是否还有足够数据可跳过
        if (position_ + count <= total_bytes_) {
            position_ += count;
            return true;
        }
        
        // 不够跳过，标记失败
        return false;
    }
    
    // 4. 返回已读取的字节数（必须实现）
    int64_t ByteCount() const override {
        return position_;
    }
    
    // 可选：重置流状态
    void Reset() {
        position_ = 0;
        total_bytes_ = buffer_->get_readable_size();
    }
    
private:
    Buffer* buffer_;
    size_t position_;      // 当前读取位置
    size_t total_bytes_;   // Buffer中总可读字节数
};
}
}
#endif