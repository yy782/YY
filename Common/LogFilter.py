def write_with_marker(filename, content, marker):

    with open(filename, 'r') as f:
        content_str = f.read()
        marker_pos = content_str.find(marker)
        
        if marker_pos == -1:
            exit("Marker not found")
    
    # 重新打开文件，写入模式
    with open(filename, 'w') as f:
        # 写入标记之前的内容
        f.write(content_str[:marker_pos + len(marker)])
        # 写入新内容
        f.write("\n")
        f.write(content)
        # 不需要截断，因为 'w' 模式会清空文件

MODULES = [
    "THREAD",
    "SIGNAL",
    "SYSTEM",
    "TCP",
    "TIME",
    "MEMORY",
    
    "EVENT",
    "HTTP",
    "DEFAULT"  # 保留默认模块
]
context = ""
context +=f"#ifndef NDEBUG\n"
for module in MODULES:
    context +=(f"#define LOG_{module}_DEBUG(msg) "
                f"LOG_BASE(LogModule::{module}, LOG_LEVEL_DEBUG, \"DEBUG\", msg)\n")
context +=f"#else\n"        
for module in MODULES:        
    context +=f"#define LOG_{module}_DEBUG(msg) \n"
context +=f"#endif\n"


for module in MODULES:
    context +=(f"#define LOG_{module}_INFO(msg) "
                f"LOG_BASE(LogModule::{module}, LOG_LEVEL_INFO, \"INFO\", msg)\n")
    context +=(f"#define LOG_{module}_WARN(msg) "
                f"LOG_BASE(LogModule::{module}, LOG_LEVEL_WARN, \"INFO\", msg)\n")            
    context +=(f"#define LOG_{module}_ERROR(msg) "
                f"LOG_BASE(LogModule::{module}, LOG_LEVEL_ERROR, \"INFO\", msg)\n")    
context +="}\n"                
context +=f"#endif"
write_with_marker("LogFilter.h",context,"//Appendfromhere:")
