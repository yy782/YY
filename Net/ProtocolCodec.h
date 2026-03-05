#ifndef _YY_NET_PROTOCOLCODEC_H_
#define _YY_NET_PROTOCOLCODEC_H_

#include <google/protobuf/message.h>
#include "../Common/Buffer.h"
#include "../Common/noncopyable.h"
#include "ProtoBufBuffer.h"
namespace yy
{
namespace net
{
class ProtocolCodec 
{
    public:
        explicit ProtocolCodec(Buffer* buffer): 
        buffer_(buffer) 
        {
            assert(buffer_ != nullptr);
            assert(buffer_->get_readable_size() == 0);
    
        }
        ~ProtocolCodec() = default;
        
        bool encode(const google::protobuf::Message& message) 
        {
            ProtoBufOutputStream outputStream(buffer_);
            return message.SerializeToZeroCopyStream(&outputStream);
        }
        
        bool decode(google::protobuf::Message& message) 
        {
            ProtoBufInputStream inputStream(buffer_);
            return message.ParseFromZeroCopyStream(&inputStream);
        }
        
        const char* data() const { return buffer_->peek(); }
        size_t size() const { return buffer_->get_readable_size(); }
        
        void reset() { buffer_->retrieveAll(); }
        
    private:
        Buffer* buffer_;
    };
}
}

#endif