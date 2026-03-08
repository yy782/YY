#include "LogAppender.h"
#include "RingBuffer.h"
#include "Types.h"
#include "AsyncLog.h"
namespace yy
{
constexpr size_t BufferSize=5;

AsyncLog::AsyncLog(const char* fileName,size_t flush_interval):
Receptionbuffer_(std::make_unique<Buffer>(BufferSize)),
SpareBuffer_(std::make_unique<Buffer>(BufferSize)),
BackstageBuffers_(),
appender_(fileName,flush_interval),
filter_(true)
{
    g_log_filter=&filter_;

    char p []="==================================== 日志启动 ==================================== \n";
    appender_.append(p,strlen(p));

    filter_.setAsynccallback(std::bind(&AsyncLog::append,this,_1));
    thread_.run(std::bind(&AsyncLog::loop,this));
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
void AsyncLog::append(const SharedString& log)
{
    assert(!log->empty());
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
            Receptionbuffer_=std::make_unique<Buffer>(BufferSize);
        }
    }
    assert(Receptionbuffer_);
    Receptionbuffer_->blockappend(log);

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
            SpareBuffer_=std::make_unique<Buffer>(BufferSize);
        }
        if(!Receptionbuffer_)
        {
            Receptionbuffer_=std::make_unique<Buffer>(BufferSize);
        }
        for(auto& buffer:WriteBuffer)
        {
            SharedString msg;
            while(buffer->retrieve(msg))
            {
                appender_.append(msg->c_str(),msg->size());
            }
        }
        WriteBuffer.clear();
    }
}   
}
