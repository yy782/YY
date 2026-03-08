#ifndef _YY_LOGAPPENDER_H_
#define _YY_LOGAPPENDER_H_

#include "noncopyable.h"
#include <stdio.h>
#include <assert.h>
#include <memory>
#include <string>
namespace yy
{

struct LowPrecisionTag;
template<typename PrecisionTag>
class TimeStamp;
template<typename PrecisionTag>
class TimeInterval;




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
    constexpr static size_t BUFFER_SIZE=64*1024;
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
    typedef TimeStamp<LowPrecisionTag> Time_Stamp;
    typedef TimeInterval<LowPrecisionTag> FlushInterval;
    typedef std::unique_ptr<Time_Stamp> Time_StampPtr;
    typedef std::unique_ptr<const FlushInterval> FlushIntervalPtr;
    LogAppender(const char* filename,size_t FlushInterval);
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
    const off_t rollSize_={2*1024*1024}; // @param 回滚大小

    Time_StampPtr lastFlushTime_;
    const FlushIntervalPtr flushInterval_;
};
}

#endif // _YY_LOGFILE_H_
