#include "SyncLog.h"
#include "TimeStamp.h"
#include "Types.h"
namespace yy
{

LogFilter* g_log_filter=nullptr;    

SyncLog::SyncLog(const char* filename,FlushInterval flush_interval):
appender_(filename,flush_interval),
filter_()
{
    g_log_filter=&filter_;
    filter_.set_callback([this](const std::string& log)
    {
        append(log);
    });
    char p []="==================================== 日志启动 ==================================== \n";
    appender_.safeAppend(p,strlen(p));
    appender_.flush(); // 立即flush
}  
void SyncLog::append(const std::string& log)
{
    appender_.safeAppend(log.c_str(),log.size());
    appender_.flush();
}
SyncLog::~SyncLog()
{
    char p []="==================================== 日志结束 ==================================== \n";
    appender_.safeAppend(p,strlen(p));
    appender_.flush();
    g_log_filter=nullptr;
} 
}
