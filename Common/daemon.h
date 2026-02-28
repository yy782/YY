#ifndef _YY_NET_DAEMON_H_
#define _YY_NET_DAEMON_H_
#include <sys/stat.h>
namespace yy
{

void daemonize(){
    pid_t pid=fork();
    if(pid==-1){
        perror("[pid] false");
        exit(1);
    }
    if(pid>0){
        exit(0);
    }
    
    if(setsid()==-1){
        exit(1);
    }
    pid=fork();//防止重新获取终端
    if(pid>0)exit(0);
    umask(0);
    //LOG_SYSTEM_INFO("[PID] "<<getpid());
    
    
    if(chdir("/")==-1){
        //LOG_SYSTEM_ERROR("[chdir] "<<"false");
        exit(1);
    }

    int null_fd=open("/dev/null",O_WRONLY);//stdcout,stdcerr重定向输出文件设备，以防cout阻塞
    if(null_fd==-1){
        //LOG_SYSTEM_ERROR("[open] "<<"false");
    }
    
    if(dup2(null_fd,STDOUT_FILENO)==-1){
        //LOG_SYSTEM_ERROR("[dup2] "<<"false");
        close(null_fd);
    }
    if(dup2(null_fd,STDERR_FILENO)==-1){
        //LOG_SYSTEM_ERROR("[dup2] "<<"false");
        close(null_fd);       
    }
    
}    
}    

#endif
