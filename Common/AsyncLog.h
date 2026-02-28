#ifndef _YY_ASYNCLOG_H_
#define _YY_ASYNCLOG_H_
#include "locker.h"
#include "noncopyable.h"
#include "LogAppender.h"
#include "LogFilter.h"
#include <memory>
#include <queue>
#include <vector>
namespace yy
{

class Buffer;



class AsyncLog :noncopyable
{
public:
    typedef std::unique_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferContainer;
    static AsyncLog& getInstance(const char* fileName,size_t flush_interval)
    {
        static AsyncLog log(fileName,flush_interval);
        return log;
    }
    ~AsyncLog();
    LogFilter* getFilter(){return &filter_;}
    LogAppender* getAppender(){return &appender_;}
private:
    AsyncLog(const char* fileName,size_t flush_interval);
    void append(const char* msg,size_t len);
    void loop();

    
    BufferPtr Receptionbuffer_;
    BufferPtr SpareBuffer_;
    BufferContainer BackstageBuffers_;
    bool isRunning_={false};
    LogAppender appender_;
    LogFilter filter_;

    
    locker Receptionlock_;
    locker BackstageLock_;
    Thread thread_;
};
}
#endif // _YY_ASYNCLOG_H_