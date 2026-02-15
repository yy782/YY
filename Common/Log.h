#ifndef LOG_H
#define LOG_H

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


enum class LogModule {
    THREAD, 
    TASK, 
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
        case LogModule::TASK: return "TASK";
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
#define LOG_THREAD_SAFE

// 日志文件路径
#define LOG_FILE_PATH "../LOG.log"

// 日志过滤器类声明
class LogFilter {
public:
    static LogFilter& get_instance(){
        static LogFilter instance;
        return instance;
    }

    LogFilter(const LogFilter&) = delete;
    LogFilter& operator=(const LogFilter&) = delete;

    void set_global_level(int level){global_level.store(level, std::memory_order_relaxed);}
    void set_module_enabled(LogModule module, bool enabled){
        std::lock_guard<std::mutex> lock(module_mutex);
        module_enabled[module] = enabled;       
    }
    bool is_module_enabled(LogModule module) const{
        std::lock_guard<std::mutex> lock(module_mutex);
        auto it = module_enabled.find(module);
        return it != module_enabled.end() ? it->second : true; // 默认启用
    }
    int get_global_level() const{return global_level.load(std::memory_order_relaxed);}
    std::mutex& get_log_mutex()const {return g_log_mutex;}
    std::ofstream& get_log_file(){return g_log_file;}
private:   
    
    LogFilter(){
        module_enabled = {
            {LogModule::THREAD, false},
            {LogModule::TASK, false},
            {LogModule::LOCK, false}, 
            {LogModule::CONFIG, false},
            {LogModule::SYSTEM, false},
            {LogModule::CLIENT, false},
            {LogModule::TIME,false},
            {LogModule::MEMORY, false},
            {LogModule::WARN, false},
            {LogModule::DEFAULT, false}
        };
        global_level.store(LOG_LEVEL_DEBUG, std::memory_order_relaxed);
    }
    
    std::unordered_map<LogModule, bool> module_enabled;
    mutable std::mutex module_mutex;
    std::atomic<int> global_level;
    mutable std::mutex g_log_mutex;
    std::ofstream g_log_file;
};


inline void set_log_global_level(int level) {
    LogFilter::get_instance().set_global_level(level);
}
inline void enable_log_module(LogModule module) {
    LogFilter::get_instance().set_module_enabled(module, true);
}

inline void disable_log_module(LogModule module) {
    LogFilter::get_instance().set_module_enabled(module, false);
}

inline int init_log_file() {
    auto& g_log_file = LogFilter::get_instance().get_log_file();
    g_log_file.open(LOG_FILE_PATH, std::ios::out | std::ios::app);
    if (!g_log_file.is_open()) {
        std::cerr << "日志文件打开失败！路径：" << LOG_FILE_PATH << std::endl;
        return -1;
    }

    g_log_file << "==================================== 日志启动 ====================================" << std::endl;
    return 1;
}

inline bool is_file_open() {
    auto& g_log_file = LogFilter::get_instance().get_log_file();
    return g_log_file.is_open();
}

inline void close_log_file() {
    auto& g_log_file = LogFilter::get_instance().get_log_file();
    if (g_log_file.is_open()) {
        g_log_file << "==================================== 日志结束 ====================================" << std::endl;
        g_log_file.close();
    }
}







// 日志宏定义
#if defined(LOG_FILE)
    #define LOG_BASE(module, level, level_str, msg) do { \
        LogFilter& filter = LogFilter::get_instance(); \
        if(filter.is_module_enabled(module) && (level >= filter.get_global_level())) { \
            std::lock_guard<std::mutex> lock(filter.get_log_mutex()); \
            auto& g_log_file = filter.get_log_file();\
            if(!g_log_file.is_open()){ \
                std::cerr<<"文件打开错误"<<std::endl; \
            } \
            g_log_file<< "[" << level_str << "] " \
                    << "[" << module_to_str(module) << "] " \
                    << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
                    << msg << std::endl; \
            g_log_file.flush(); \
        } \
    } while(0)
#else
    #define LOG_BASE(module, level, level_str, msg) do { \
        LogFilter& filter = LogFilter::get_instance(); \
        if (filter.is_module_enabled(module) && (level >= filter.get_global_level())) { \
            std::lock_guard<std::mutex> lock(filter.get_log_mutex()); \
            std::cout << "[" << level_str << "] " \
                    << "[" << module_to_str(module) << "] " \
                    << "[" << __FILENAME__ << ":" << __LINE__ << "] " \
                    << msg << std::endl; \
        } \
    } while(0)
#endif

#ifdef COUT
    #define LOG_THREAD_DEBUG(msg) LOG_BASE(LogModule::THREAD, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_THREAD_INFO(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_THREAD_WARN(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_WARN,  "WARN",  msg) 
    #define LOG_THREAD_ERROR(msg)  LOG_BASE(LogModule::THREAD, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_LOCK_DEBUG(msg) LOG_BASE(LogModule::LOCK, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_LOCK_INFO(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_LOCK_WARN(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_WARN,  "WARN",  msg) 
    #define LOG_LOCK_ERROR(msg)  LOG_BASE(LogModule::LOCK, LOG_LEVEL_ERROR,  "ERROR",  msg)

    #define LOG_TASK_DEBUG(msg) LOG_BASE(LogModule::TASK, LOG_LEVEL_DEBUG, "DEBUG", msg)
    #define LOG_TASK_INFO(msg)  LOG_BASE(LogModule::TASK, LOG_LEVEL_INFO,  "INFO",  msg)
    #define LOG_TASK_WARN(msg)  LOG_BASE(LogModule::TASK, LOG_LEVEL_WARN,  "WARN",  msg)  
    #define LOG_TASK_ERROR(msg)  LOG_BASE(LogModule::TASK, LOG_LEVEL_ERROR,  "ERROR",  msg)

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
#else
    #define LOG_THREAD_DEBUG(msg) 
    #define LOG_THREAD_INFO(msg)  
    #define LOG_THREAD_WARN(msg)  
    #define LOG_THREAD_ERROR(msg)  

    #define LOG_LOCK_DEBUG(msg) 
    #define LOG_LOCK_INFO(msg)  
    #define LOG_LOCK_WARN(msg)  
    #define LOG_LOCK_ERROR(msg)  

    #define LOG_TASK_DEBUG(msg) 
    #define LOG_TASK_INFO(msg)  
    #define LOG_TASK_WARN(msg)  
    #define LOG_TASK_ERROR(msg)  

    #define LOG_SYSTEM_DEBUG(msg) 
    #define LOG_SYSTEM_INFO(msg) 
    #define LOG_SYSTEM_WARN(msg)  
    #define LOG_SYSTEM_ERROR(msg)  

    #define LOG_CLIENT_DEBUG(msg) 
    #define LOG_CLIENT_INFO(msg)  
    #define LOG_CLIENT_WARN(msg)
    #define LOG_CLIENT_ERROR(msg)  

    #define LOG_TIME_DEBUG(msg) 
    #define LOG_TIME_INFO(msg)  
    #define LOG_TIME_WARN(msg)  
    #define LOG_TIME_ERROR(msg)  
    
    #define LOG_MEMORY_ERROR(msg)
    #define LOG_MEMORY_WARN(msg)
    #define LOG_MEMORY_INFO(msg)
    #define LOG_MEMORY_DEBUG(msg)

    #define LOG_NULL_WARN(msg)
    

    
#endif







#endif // LOG_H