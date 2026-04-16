#ifndef _YY_SYNCLOG_H_
#define _YY_SYNCLOG_H_

#include "LogAppender.h"
#include "LogFilter.h"
#include "noncopyable.h"
namespace yy
{
class SyncLog:noncopyable
{
public:
    typedef LogAppender::FlushInterval FlushInterval;
    LogFilter& getFilter(){return filter_;}
    LogAppender& getAppender(){return appender_;}
    static SyncLog& getInstance(const char* filename)
    {
        static SyncLog instance(filename);
        return instance;
    }
    ~SyncLog();
private:
    SyncLog(const char* filename);
    void append(const std::string& log);
    LogAppender appender_;
    LogFilter filter_;
};

}


#endif