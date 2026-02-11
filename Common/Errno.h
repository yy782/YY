#include <errno.h>

#include "Log.h"


#define LOG_PRINT_ERRNO int save_errno=errno;\
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
                        case EADDRINUSE:\
                            LOG_NULL_WARN("地址已被占用");\
                            break;\
                        default:\
                            LOG_NULL_WARN("[errno] "<<errno);\
                            break;\
                        }
