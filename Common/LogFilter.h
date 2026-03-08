#ifndef _YY_LOGFILER_H_
#define _YY_LOGFILER_H_

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <cstdio>
#include <functional>
#include <assert.h>
#include <sstream>
#include "noncopyable.h"

#include "Types.h"

#include <string>
namespace yy
{

class LogStream 
{
public:
    LogStream() : oss_(std::make_shared<std::string>()) {}
    LogStream& operator<<(short val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(unsigned short val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(int val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(unsigned int val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(long val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(unsigned long val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(long long val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(unsigned long long val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(float val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(double val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(long double val) {
        *oss_ += std::to_string(val);
        return *this;
    }
    
    LogStream& operator<<(bool val) {
        *oss_ += val ? "true" : "false";
        return *this;
    }
    
    LogStream& operator<<(char val) {
        oss_->push_back(val);
        return *this;
    }
    
    LogStream& operator<<(const char* val) {
        assert(val);
        *oss_ += val;
        
        return *this;
    }
    
    LogStream& operator<<(const std::string& val) {
        *oss_ += val;
        return *this;
    }
    
    LogStream& operator<<(const std::string_view& val) {
        *oss_ += val;
        return *this;
    }
    SharedString str() const{
        return oss_;
    }

private:
    SharedString oss_;
};


enum class LogModule 
{
    THREAD, 
    SIGNAL, 
    LOCK, 
    CONFIG, 
    SYSTEM, 
    CLIENT,
    TIME,
    MEMORY,
    WARN,
    DEFAULT
};

inline const char* module_to_str(LogModule module) {
    switch (module) {
        case LogModule::THREAD: return "THREAD";
        case LogModule::SIGNAL: return "SIGNAL";
        case LogModule::LOCK: return "LOCK";
        case LogModule::CONFIG: return "CONFIG";
        case LogModule::SYSTEM: return "SYSTEM";
        case LogModule::CLIENT: return "CLIENT";
        case LogModule::TIME: return "TIME";
        case LogModule::MEMORY: return "MEMORY";
        case LogModule::WARN: return "WARN";
        default: return "DEFAULT";
    }
}

// 日志级别定义
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)


class LogFilter:noncopyable // 只负责过滤，输出通过回调指向
{
public:
    typedef std::function<void(const SharedString&)> AsyncLogCallback;
    typedef std::function<void(const char* msg,size_t len)> SyncLogCallback;
    LogFilter(bool is_async):
    global_level_(LOG_LEVEL_DEBUG),
    is_async_(is_async)
    {
        module_enabled_={
            {LogModule::THREAD, false},
            {LogModule::SIGNAL, false},
            {LogModule::LOCK, false}, 
            {LogModule::CONFIG, false},
            {LogModule::SYSTEM, false},
            {LogModule::CLIENT, false},
            {LogModule::TIME,false},
            {LogModule::MEMORY, false},
            {LogModule::WARN, false},
            {LogModule::DEFAULT, false}
        };
    }
    void set_global_level(int level){global_level_=level;}
    void set_module_enabled(LogModule module, bool enabled){
        module_enabled_[module] = enabled;       
    }
    void setSynccallback(SyncLogCallback callback){
        Synccallback_=std::move(callback);
    }
    void setAsynccallback(AsyncLogCallback callback){
        Asynccallback_=std::move(callback);
    }
    bool is_module_enabled(LogModule module) const{
        auto it = module_enabled_.find(module);
        return it != module_enabled_.end() ? it->second : true; // 默认启用
    }
    int get_global_level() const{return global_level_;}
    void printLog(SharedString data)
    {
        if(is_async_)
            Asynccallback_(data);
        else
            Synccallback_(data->c_str(),data->size());    
    }
private:  
    std::unordered_map<LogModule, bool> module_enabled_;
    int global_level_;
    SyncLogCallback Synccallback_;
    AsyncLogCallback Asynccallback_;
    const bool is_async_;
};

extern LogFilter* g_log_filter;


#define LOG_BASE(module, level, level_str, msg) do { \
    if(g_log_filter){ \
        if(g_log_filter->is_module_enabled(module) && (level >= g_log_filter->get_global_level())) { \
            LogStream stream; \
            stream << "[" << level_str << "] " \
                << "[" << module_to_str(module) << "] " \
                << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
                << msg << "\n"; \
            g_log_filter->printLog(stream.str()); \
        } \
    }\
} while(0)




//#ifdef COUT
    #define LOG_THREAD_DEBUG(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_THREAD_INFO(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_THREAD_WARN(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_WARN,  "WARN",  msg) 
    #define LOG_THREAD_ERROR(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_LOCK_DEBUG(msg) LOG_BASE(LogModule::LOCK, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_LOCK_INFO(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_LOCK_WARN(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_WARN,  "WARN",  msg) 
    #define LOG_LOCK_ERROR(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_SIGNAL_DEBUG(msg) LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_SIGNAL_INFO(msg)  LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_SIGNAL_WARN(msg)  LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_WARN,  "WARN",  msg)  
    #define LOG_SIGNAL_ERROR(msg)  LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_SYSTEM_DEBUG(msg) LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_SYSTEM_INFO(msg)  LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_SYSTEM_WARN(msg)  LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_WARN,  "WARN",  msg)  
    #define LOG_SYSTEM_ERROR(msg)  LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_CLIENT_DEBUG(msg) LOG_BASE(LogModule::CLIENT, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_CLIENT_INFO(msg)  LOG_BASE(LogModule::CLIENT, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_CLIENT_WARN(msg)  LOG_BASE(LogModule::CLIENT, LOG_LEVEL_WARN,  "WARN",  msg)  
    #define LOG_CLIENT_ERROR(msg)  LOG_BASE(LogModule::CLIENT, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_TIME_DEBUG(msg) LOG_BASE(LogModule::TIME, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_TIME_INFO(msg)  LOG_BASE(LogModule::TIME, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_TIME_WARN(msg)  LOG_BASE(LogModule::TIME, LOG_LEVEL_WARN,  "WARN",  msg)
    #define LOG_TIME_ERROR(msg)  LOG_BASE(LogModule::TIME, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_MEMORY_DEBUG(msg) LOG_BASE(LogModule::MEMORY, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_MEMORY_INFO(msg)  LOG_BASE(LogModule::MEMORY, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_MEMORY_WARN(msg)  LOG_BASE(LogModule::MEMORY, LOG_LEVEL_WARN,  "WARN",  msg)
    #define LOG_MEMORY_ERROR(msg)  LOG_BASE(LogModule::MEMORY, LOG_LEVEL_ERROR,  "ERROR",  msg)    


    #define LOG_NULL_WARN(msg)     LOG_BASE(LogModule::WARN, LOG_LEVEL_WARN,  "WARN",  msg) 

#define LOG_PRINT_ERRNO(save_errno) \
                        switch(save_errno){\
                        case EBADF:\
                            LOG_NULL_WARN("非法的文件描述符");\
                            break;\
                         case ENOTSOCK:\
                            LOG_NULL_WARN("FD不是套接字");\
                            break;\
                        case EINVAL:\
                            LOG_NULL_WARN("参数无效");\
                            break;\
                        case ENOPROTOOPT:\
                            LOG_NULL_WARN("协议不支持该选项");\
                            break;\
                        case EPERM:\
                            LOG_NULL_WARN("权限不足");\
                            break;\
                        case EFAULT:\
                            LOG_NULL_WARN("地址非法");\
                            break;\
                        case EINTR:\
                            LOG_NULL_WARN("系统调用被信号中断");\
                            break;\
                        case EADDRINUSE:\
                            LOG_NULL_WARN("地址已被占用");\
                            break;\
                        case EPIPE:\
                            LOG_NULL_WARN("管道破裂");\
                            break;\
                        default:\
                            LOG_NULL_WARN("[errno] "<<errno);\
                            break;\
                        }


// #else
//     #define LOG_THREAD_DEBUG(msg) 
//     #define LOG_THREAD_INFO(msg)  
//     #define LOG_THREAD_WARN(msg)  
//     #define LOG_THREAD_ERROR(msg)  

//     #define LOG_LOCK_DEBUG(msg) 
//     #define LOG_LOCK_INFO(msg)  
//     #define LOG_LOCK_WARN(msg)  
//     #define LOG_LOCK_ERROR(msg)  

//     #define LOG_SIGNAL_DEBUG(msg) 
//     #define LOG_SIGNAL_INFO(msg)  
//     #define LOG_SIGNAL_WARN(msg)  
//     #define LOG_SIGNAL_ERROR(msg)  

//     #define LOG_SYSTEM_DEBUG(msg) 
//     #define LOG_SYSTEM_INFO(msg) 
//     #define LOG_SYSTEM_WARN(msg)  
//     #define LOG_SYSTEM_ERROR(msg)  

//     #define LOG_CLIENT_DEBUG(msg) 
//     #define LOG_CLIENT_INFO(msg)  
//     #define LOG_CLIENT_WARN(msg)
//     #define LOG_CLIENT_ERROR(msg)  

//     #define LOG_TIME_DEBUG(msg) 
//     #define LOG_TIME_INFO(msg)  
//     #define LOG_TIME_WARN(msg)  
//     #define LOG_TIME_ERROR(msg)  
    
//     #define LOG_MEMORY_ERROR(msg)
//     #define LOG_MEMORY_WARN(msg)
//     #define LOG_MEMORY_INFO(msg)
//     #define LOG_MEMORY_DEBUG(msg)

//     #define LOG_NULL_WARN(msg)
    

    
// #endif






}
#endif // LOG_H