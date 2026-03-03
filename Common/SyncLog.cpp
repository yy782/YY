#include "SyncLog.h"
#include "TimeStamp.h"
#include "Types.h"
namespace yy
{

LogFilter* g_log_filter=nullptr;    

SyncLog::SyncLog(const char* fileName,size_t flush_interval):
appender_(fileName,flush_interval),
filter_()
{
    g_log_filter=&filter_;
    filter_.set_log_file_callback(std::bind(&LogAppender::append,std::ref(appender_),_1,_2));
    char p []="==================================== 日志启动 ==================================== \n";
    appender_.append(p,strlen(p));
}  
SyncLog::~SyncLog()
{
    char p []="==================================== 日志结束 ==================================== \n";
    appender_.append(p,strlen(p));
    appender_.flush();
    g_log_filter=nullptr;
} 
}