#include "LogAppender.h"
#include "RingBuffer.h"
#include "Types.h"
#include "AsyncLog.h"
#include "stringPiece.h"
namespace yy
{


AsyncLog::AsyncLog(const char* fileName,LTimeInterval flush_interval,size_t BufferSize):
BufferSize_(BufferSize),
Receptionbuffer_(std::make_unique<Buffer>(BufferSize)),
SpareBuffer_(std::make_unique<Buffer>(BufferSize)),
BackstageBuffers_(),
appender_(fileName,flush_interval),
filter_()
{
    g_log_filter=&filter_;

    char p []="==================================== 日志启动 ==================================== \n";
    appender_.append(p,strlen(p));

    filter_.set_callback([this](const std::string& log)
    {
        append(log);
    });
    thread_.run([this]()
    {
        loop();
    });
}  
AsyncLog::~AsyncLog()
{
    isRunning_=false;
    if(thread_.joinable())
    {
        thread_.join();
    }
    char p []="==================================== 日志结束 ==================================== \n";
    appender_.append(p,strlen(p));
    appender_.flush(); 
}
void AsyncLog::append(const std::string& log)
{
    assert(!log.empty());
    if(Receptionbuffer_->full())
    {
        BackstageLock_.lock();
        BackstageBuffers_.push_back(std::move(Receptionbuffer_));
        BackstageLock_.unlock();

        if(SpareBuffer_)
        {
            Receptionbuffer_=std::move(SpareBuffer_);
        }
        else
        {
            Receptionbuffer_=std::make_unique<Buffer>(BufferSize_);
        }
    }
    assert(Receptionbuffer_);
    if(!Receptionbuffer_->append(log))
    {
        SecondaryBuffer_.push_back(log);
    }
}
void AsyncLog::loop()
{
    isRunning_=true;
    BufferContainer WriteBuffer;
    while(isRunning_)
    {
        BackstageLock_.lock();
        if(!BackstageBuffers_.empty())
        {

            WriteBuffer.swap(BackstageBuffers_);
        }
        BackstageLock_.unlock();
        if(!SpareBuffer_)
        {
            SpareBuffer_=std::make_unique<Buffer>(BufferSize_);
        }
        if(!Receptionbuffer_)
        {
            Receptionbuffer_=std::make_unique<Buffer>(BufferSize_);
        }
        std::string msg;
        for(auto& buffer:WriteBuffer)
        {
            if(buffer==nullptr)continue;
            while(buffer->retrieve(msg))
            {
                appender_.append(msg.data(),msg.size());
            }
        }
        WriteBuffer.clear();

        SecondaryBuffer temp=SecondaryBuffer_.getAndClear();
        for(auto& log:temp)
        {
            appender_.append(log.data(),log.size());
        }
    }
}   
}
