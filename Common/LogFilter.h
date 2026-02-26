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


namespace yy
{

class LogStream 
{
public:
  template <typename T>
  LogStream& operator<<(const T& val) {
    oss_ << val;
    return *this;
  }
  std::string str()const{return oss_.str();}
  size_t size()const{
    return oss_.str().size(); 
  }
private:
  std::ostringstream oss_;
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
    LogFilter():
    global_level(LOG_LEVEL_DEBUG)
    {
        module_enabled={
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
    void set_global_level(int level){global_level=level;}
    void set_module_enabled(LogModule module, bool enabled){
        module_enabled[module] = enabled;       
    }
    void set_log_file_callback(std::function<void(const char* data,size_t len)> callback){
        log_file_callback=std::move(callback);
    }
    bool is_module_enabled(LogModule module) const{
        auto it = module_enabled.find(module);
        return it != module_enabled.end() ? it->second : true; // 默认启用
    }
    int get_global_level() const{return global_level;}
    void printLog(const char* data, size_t len)
    {
        assert(log_file_callback);
        log_file_callback(data,len);
    }
private:  
    std::unordered_map<LogModule, bool> module_enabled;
    int global_level;
    std::function<void(const char* data,size_t len)> log_file_callback;
};

extern LogFilter* g_log_filter;


#define LOG_BASE(module, level, level_str, msg) do { \
    assert(g_log_filter); \
    if(g_log_filter->is_module_enabled(module) && (level >= g_log_filter->get_global_level())) { \
        LogStream stream; \
        stream << "[" << level_str << "] " \
               << "[" << module_to_str(module) << "] " \
               << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
               << msg << "\n"; \
        g_log_filter->printLog(stream.str().c_str(), stream.size()); \
    } \
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