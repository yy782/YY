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
#include <list>
#include <set>
#include "stringPiece.h"
namespace yy
{

class LogStream 
{
public:
    LogStream() = default;
    
    // 使用模板统一处理所有可以通过ostringstream输出的类型
    template<typename T>
    LogStream& operator<<(const T& val) {
        oss_ << val;
        return *this;
    }
    LogStream& operator<<(const stringPiece& val) {
        oss_.write(val.data(),val.size());
        return *this;
    }
    
    // 特化bool类型输出为true/false
    LogStream& operator<<(bool val) {
        oss_ << (val ? "true" : "false");
        return *this;
    }
    
    // 特化const char*以处理空指针
    LogStream& operator<<(const char* val) {
        if (val == nullptr) {
            oss_ << "(null)";
        } else {
            oss_ << val;
        }
        return *this;
    }
    
    // 特化char*以处理非const指针
    LogStream& operator<<(char* val) {
        return operator<<(const_cast<const char*>(val));
    }
    
    // 获取当前字符串内容
    std::string str() const {
        return oss_.str();
    }
    
    // 清空流内容
    void clear() {
        oss_.str("");
        oss_.clear();
    }

private:
    std::ostringstream oss_;
};

struct LogModule 
{
    static inline const std::string THREAD = "THREAD";
    static inline const std::string SIGNAL = "SIGNAL";
    static inline const std::string SYSTEM = "SYSTEM";
    static inline const std::string TCP = "TCP";
    static inline const std::string TIME = "TIME";
    static inline const std::string MEMORY = "MEMORY";
    static inline const std::string EVENT = "EVENT";
    static inline const std::string WARN = "WARN";
    static inline const std::string HTTP = "HTTP";
    static inline const std::string CLIENT = "CLIENT";
    static inline const std::string LOOP = "LOOP";
    static inline const std::string DEFAULT = "DEFAULT";
};



// 日志级别定义
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)


class LogFilter:noncopyable // 只负责过滤，输出通过回调指向
{
public:
    typedef std::function<void(const std::string&)> LogCallback;
    
    LogFilter():
    global_level_(LOG_LEVEL_DEBUG)
    {
    }
    LogFilter& set_global_level(int level)
    {
        global_level_=level;
        return *this;
    }
    LogFilter& set_global_level(const std::string& level)
    {
        assert(std::all_of(level.begin(), level.end(),[](unsigned char c) {
        return std::isupper(c) || std::isspace(c) || std::ispunct(c);}));
         global_level_ = level == "DEBUG" ? LOG_LEVEL_DEBUG :
                         level == "INFO" ? LOG_LEVEL_INFO :
                         level == "WARN" ? LOG_LEVEL_WARN :
                         level == "ERROR" ? LOG_LEVEL_ERROR :
                         LOG_LEVEL_DEBUG;
        return *this;
    }
    LogFilter& set_global_level(std::string& level)
    {
        std::transform(level.begin(),level.end(),level.begin(), ::toupper);
         global_level_ = level == "DEBUG" ? LOG_LEVEL_DEBUG :
                         level == "INFO" ? LOG_LEVEL_INFO :
                         level == "WARN" ? LOG_LEVEL_WARN :
                         level == "ERROR" ? LOG_LEVEL_ERROR :
                         LOG_LEVEL_DEBUG;
        return *this;
    }
    LogFilter& set_module_enabled(std::string& module)
    {
        std::transform(module.begin(),module.end(),module.begin(), ::toupper);
        module_enabled_.insert(module); 
        return *this;      
    }
    LogFilter& set_module_enabled(const std::string& module)
    {
        assert(std::all_of(module.begin(), module.end(),::isupper)&& 
            "module must be uppercase");
        
        module_enabled_.insert(module);
        return *this;
    }
    LogFilter& set_module_enabled(std::list<std::string>& modules)
    {
        for (auto& module_name : modules) 
        {
            std::transform(module_name.begin(), module_name.end(), module_name.begin(), ::toupper);
            module_enabled_.insert(module_name);
        }   
        return *this;
    }
    int get_global_level() const
    {
        return global_level_;
    }
    void set_callback(LogCallback callback){
        callback_=std::move(callback);
    }
    bool is_module_enabled(const std::string& module) const // 注意没有转换
    {
        return module_enabled_.find(module) != module_enabled_.end();
    }
    void printLog(const std::string& data)
    {
        assert(callback_);
        callback_(data);
    }
private:  
    std::set<std::string> module_enabled_;
    int global_level_;
    LogCallback callback_;
};

extern LogFilter* g_log_filter;

#ifndef NDEBUG
    #define LOG_BASE(module, level, level_str, msg) do { \
        if(g_log_filter){ \
            if(g_log_filter->is_module_enabled(module) && (level >= g_log_filter->get_global_level())) { \
                LogStream stream; \
                stream << "[" << level_str << "] " \
                    << "[" << module << "] " \
                    << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
                    << msg << "\n"; \
                g_log_filter->printLog(stream.str()); \
            } \
        }\
    } while(0)
#else 
    #define LOG_BASE(module, level, level_str, msg) do { \
        if(g_log_filter){ \
            { \
                LogStream stream; \
                stream << "[" << level_str << "] " \
                    << "[" << module << "] " \
                    << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
                    << msg << "\n"; \
                g_log_filter->printLog(stream.str()); \
            } \
        }\
    } while(0)
#endif  

 




#ifdef NEXCLUDE_BEFORE_COMPILATION
    #define EXCLUDE_BEFORE_COMPILATION(msg) msg
#else 
    #define EXCLUDE_BEFORE_COMPILATION(msg) 
#endif 


    #define LOG_WARN(msg)     LOG_BASE(LogModule::WARN, LOG_LEVEL_WARN,  "WARN",  msg)
    #define LOG_SYSFATAL(msg)       LOG_BASE(LogModule::DEFAULT, LOG_LEVEL_ERROR, "SYSTEM", msg); \
                                    assert(false);    

#define LOG_ERRNO(save_errno) \
                        switch(save_errno){\
                        case EBADF:\
                            LOG_WARN("非法的文件描述符");\
                            break;\
                         case ENOTSOCK:\
                            LOG_WARN("FD不是套接字");\
                            break;\
                        case EINVAL:\
                            LOG_WARN("参数无效");\
                            break;\
                        case ENOPROTOOPT:\
                            LOG_WARN("协议不支持该选项");\
                            break;\
                        case EPERM:\
                            LOG_WARN("权限不足");\
                            break;\
                        case EFAULT:\
                            LOG_WARN("地址非法");\
                            break;\
                        case EINTR:\
                            LOG_WARN("系统调用被信号中断");\
                            break;\
                        case EADDRINUSE:\
                            LOG_WARN("地址已被占用");\
                            break;\
                        case EPIPE:\
                            LOG_WARN("管道破裂");\
                            break;\
                        default:\
                            LOG_WARN("[errno] "<<errno);\
                            break;\
                        }



//Appendfromhere:
#ifndef NDEBUG
#define LOG_THREAD_DEBUG(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_SIGNAL_DEBUG(msg) LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_SYSTEM_DEBUG(msg) LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_TCP_DEBUG(msg) LOG_BASE(LogModule::TCP, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_TIME_DEBUG(msg) LOG_BASE(LogModule::TIME, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_MEMORY_DEBUG(msg) LOG_BASE(LogModule::MEMORY, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_CLIENT_DEBUG(msg) LOG_BASE(LogModule::CLIENT, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_EVENT_DEBUG(msg) LOG_BASE(LogModule::EVENT, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_HTTP_DEBUG(msg) LOG_BASE(LogModule::HTTP, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_LOOP_DEBUG(msg) LOG_BASE(LogModule::LOOP, LOG_LEVEL_DEBUG, "DEBUG", msg)
#define LOG_DEFAULT_DEBUG(msg) LOG_BASE(LogModule::DEFAULT, LOG_LEVEL_DEBUG, "DEBUG", msg)
#else
#define LOG_THREAD_DEBUG(msg) 
#define LOG_SIGNAL_DEBUG(msg) 
#define LOG_SYSTEM_DEBUG(msg) 
#define LOG_TCP_DEBUG(msg) 
#define LOG_TIME_DEBUG(msg) 
#define LOG_MEMORY_DEBUG(msg) 
#define LOG_CLIENT_DEBUG(msg) 
#define LOG_EVENT_DEBUG(msg) 
#define LOG_HTTP_DEBUG(msg) 
#define LOG_LOOP_DEBUG(msg) 
#define LOG_DEFAULT_DEBUG(msg) 
#endif
#define LOG_THREAD_INFO(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_THREAD_WARN(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_THREAD_ERROR(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_SIGNAL_INFO(msg) LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_SIGNAL_WARN(msg) LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_SIGNAL_ERROR(msg) LOG_BASE(LogModule::SIGNAL, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_SYSTEM_INFO(msg) LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_SYSTEM_WARN(msg) LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_SYSTEM_ERROR(msg) LOG_BASE(LogModule::SYSTEM, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_TCP_INFO(msg) LOG_BASE(LogModule::TCP, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_TCP_WARN(msg) LOG_BASE(LogModule::TCP, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_TCP_ERROR(msg) LOG_BASE(LogModule::TCP, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_TIME_INFO(msg) LOG_BASE(LogModule::TIME, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_TIME_WARN(msg) LOG_BASE(LogModule::TIME, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_TIME_ERROR(msg) LOG_BASE(LogModule::TIME, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_MEMORY_INFO(msg) LOG_BASE(LogModule::MEMORY, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_MEMORY_WARN(msg) LOG_BASE(LogModule::MEMORY, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_MEMORY_ERROR(msg) LOG_BASE(LogModule::MEMORY, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_CLIENT_INFO(msg) LOG_BASE(LogModule::CLIENT, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_CLIENT_WARN(msg) LOG_BASE(LogModule::CLIENT, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_CLIENT_ERROR(msg) LOG_BASE(LogModule::CLIENT, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_EVENT_INFO(msg) LOG_BASE(LogModule::EVENT, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_EVENT_WARN(msg) LOG_BASE(LogModule::EVENT, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_EVENT_ERROR(msg) LOG_BASE(LogModule::EVENT, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_HTTP_INFO(msg) LOG_BASE(LogModule::HTTP, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_HTTP_WARN(msg) LOG_BASE(LogModule::HTTP, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_HTTP_ERROR(msg) LOG_BASE(LogModule::HTTP, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_LOOP_INFO(msg) LOG_BASE(LogModule::LOOP, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_LOOP_WARN(msg) LOG_BASE(LogModule::LOOP, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_LOOP_ERROR(msg) LOG_BASE(LogModule::LOOP, LOG_LEVEL_ERROR, "ERROR", msg)
#define LOG_DEFAULT_INFO(msg) LOG_BASE(LogModule::DEFAULT, LOG_LEVEL_INFO, "INFO", msg)
#define LOG_DEFAULT_WARN(msg) LOG_BASE(LogModule::DEFAULT, LOG_LEVEL_WARN, "WARN", msg)
#define LOG_DEFAULT_ERROR(msg) LOG_BASE(LogModule::DEFAULT, LOG_LEVEL_ERROR, "ERROR", msg)
}
#endif