#include "LogAppender.h"

#include <assert.h>
#include <iostream>
#include <errno.h>



namespace yy
{

static std::string getFileName(const char* filename)
{
    std::string filenameStr(filename);
    size_t pos=filenameStr.find_last_of('.');
    return filenameStr.substr(0,pos);
}
static std::string getSuffix(const char* filename)
{
    std::string filenameStr(filename);
    size_t pos=filenameStr.find_last_of('.');
    return filenameStr.substr(pos);
}

BaseLogAppender::BaseLogAppender(const char* filename):
name_(getFileName(filename).c_str()),
type_(getSuffix(filename).c_str()),
fp_(::fopen(filename,"a")), // e是当进程执行FD_CLOEXEC时关闭
writtenBytes_(0)
{
    if(fp_==nullptr)
    {
        exit(1);
    }
    // @brief 替换掉标准库的默认缓冲区，默认缓冲区太小了
    ::setvbuf(fp_, buffer_, _IOFBF, BUFFER_SIZE);
}  
BaseLogAppender::~BaseLogAppender()
{

    ::fclose(fp_);
    
}
void BaseLogAppender::append(const char* logline,size_t len)
{
    size_t written =0;


    while (written != len)
    {
        size_t remain =len-written;
        size_t n=fwrite(logline + written,1,remain,fp_);// @param 第二个参数是单个数据项的字节数  ,默认是线程安全的
        if (n!=remain)
        {
            int err=ferror(fp_);
            if(err)
            {
                // @TODO
                break;
            }
        }
    written+=n;
    }
    writtenBytes_+=len;
}
void BaseLogAppender::flush()
{
    ::fflush(fp_);
}
LogAppender::LogAppender(const char* filename,LTimeInterval flush_interval):
baseLogFile_(std::make_unique<BaseLogAppender>(filename)),
logFileName_(getFileName(filename)),
flushInterval_(flush_interval)
{
    assert(filename);
}
LogAppender::~LogAppender()
{

}
void LogAppender::append(const char* logline, size_t len)
{
    baseLogFile_->append(logline,len);
    if(baseLogFile_->getWrittenBytes()>rollSize_)
    {
        rollFile();
    }
    else
    {
        Timeflush();
    }
}
void LogAppender::safeAppend(const char* logline, size_t len)
{
    std::lock_guard<std::mutex> l(mtx_);
    if (fileno(baseLogFile_->getFp()) < 0) {
        fprintf(stderr, "Invalid file descriptor in log\n");
        rollFile();
    }
    baseLogFile_->append(logline,len);
    if(baseLogFile_->getWrittenBytes()>rollSize_)
    {
        rollFile();
    }
    else
    {
        Timeflush();
    }
}
void LogAppender::rollFile()
{
    auto filename=getLogFileName();
    baseLogFile_.reset(new BaseLogAppender(filename.c_str()));
}
void LogAppender::Timeflush()
{
    static size_t count=0;
    static size_t checkInterval=5;
    ++count;
    if(count<checkInterval)return;// @brief 为了减少系统调用，减少性能开销
    auto time=LTimeStamp(LTimeStamp::now());
    static LTimeStamp NextFlushTime=time+flushInterval_;

    if(time>NextFlushTime)
    {
        baseLogFile_->flush();
        NextFlushTime=time;
    }
}
std::string LogAppender::getLogFileName()
{
    std::string filename;
    auto suffix=baseLogFile_->getTyps();
    filename.reserve(logFileName_.size()+32+suffix.size());
    filename=logFileName_;
    auto nowTime=LTimeStamp::nowToString();
    filename+=nowTime;
    filename+=suffix;
    return filename;
    
}
}
