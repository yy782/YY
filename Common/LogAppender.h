#ifndef _YY_LOGAPPENDER_H_
#define _YY_LOGAPPENDER_H_
#include "TimeStamp.h"
#include "noncopyable.h"
#include <stdio.h>
#include <assert.h>
#include <memory>
#include <string>

using namespace std::chrono_literals;

namespace yy
{

class BaseLogAppender:noncopyable
{
public:

    BaseLogAppender(const char* filename);
    ~BaseLogAppender();
    void append(const char* logline,size_t len);
    void flush();
    off_t getWrittenBytes()const{return writtenBytes_;}
    const std::string& getName()const{return name_;}
    const std::string& getTyps()const{return type_;}
    constexpr static size_t BUFFER_SIZE=6;
private:
    const std::string name_;
    const std::string type_; // @param 文件类型
    FILE* fp_;
    off_t writtenBytes_;
    // @param 使用off_t类型，因为文件大小可能超过int表示的范围,而且off_t是平台相关的
    char buffer_[BUFFER_SIZE];
};

class LogAppender:noncopyable
{//添加回滚操作
public:
    typedef LTimeInterval FlushInterval;
    LogAppender(const char* filename,LTimeInterval flush_interval=10s);
    ~LogAppender();
    typedef std::unique_ptr<BaseLogAppender> BaseLogFilePtr;
    void append(const char* logline, size_t len);
    void flush(){baseLogFile_->flush();}
    void rollFile();
private:
    
    void Timeflush();
    std::string getLogFileName();
    BaseLogFilePtr baseLogFile_;
    const std::string logFileName_;
    const off_t rollSize_={2*1024}; // @param 回滚大小

    LTimeStamp lastFlushTime_;
    const FlushInterval flushInterval_;
};
}

#endif // _YY_LOGFILE_H_
