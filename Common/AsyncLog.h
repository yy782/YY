#ifndef _YY_ASYNCLOG_H_
#define _YY_ASYNCLOG_H_
#include "locker.h"
#include "noncopyable.h"
#include "LogAppender.h"
#include "LogFilter.h"
#include "RingBuffer.h"
#include <memory>
#include <queue>
#include <vector>
#include <string>
#include "Types.h"
#include <chrono>
#include "SafeVector.h"
#include "TimeStamp.h"
using namespace std::chrono_literals;
namespace yy
{

class AsyncLog:noncopyable
{
public:
    typedef RingBuffer<std::string> Buffer;
    typedef SafeVector<std::string> SecondaryBuffer;
    typedef std::unique_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferContainer;
    static AsyncLog& getInstance(const char* fileName,LTimeInterval flush_interval=10s,size_t BufferSize=20)
    {
        static AsyncLog log(fileName,flush_interval,BufferSize);
        return log;
    }
    ~AsyncLog();
    LogFilter& getFilter(){return filter_;}
    LogAppender& getAppender(){return appender_;}
private:
    AsyncLog(const char* fileName,LTimeInterval flush_interval=10s,size_t BufferSize=20);
    void append(const std::string& data);
    void loop();

    size_t BufferSize_;
    BufferPtr Receptionbuffer_;
    SecondaryBuffer SecondaryBuffer_;
    BufferPtr SpareBuffer_;
    BufferContainer BackstageBuffers_;
    bool isRunning_={false};
    LogAppender appender_;
    LogFilter filter_;

    locker BackstageLock_;
    Thread thread_;
};
}
#endif // _YY_ASYNCLOG_H_