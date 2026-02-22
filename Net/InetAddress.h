#ifndef _YY_NET_INETADDRESS_
#define _YY_NET_INETADDRESS_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "../Common/copyable.h"
#include "../Common/Log.h"
#include "../Common/Types.h"
namespace yy
{
namespace net
{
class Address:public copyable
{
    
public:
    Address()=default;

    explicit Address(uint16_t port,bool loopbackOnly=false,bool ipv6=false);

    Address& operator=(const struct sockaddr_in6& addr)
    {
        addr6_=addr;
        return *this;
    }
    Address& operator=(const struct sockaddr_in& addr)
    {
        addr_=addr;
        return *this;
    }
    Address(const char* ip,uint16_t port,bool ipv6=false);
    void set_ipv6_addr(const struct sockaddr_in6& addr6){addr6_=addr6;}
    void set_ipv4_addr(const struct sockaddr_in& addr){addr_=addr;}
    void get_ip(char* ip)const
    {
        assert(inet_ntop(AF_INET,&(addr_.sin_addr),ip,sizeof(ip))!=NULL&&"IP地址转换可读的字符串失败");
    }
    const std::string get_ip()const
    {
        char ip[INET_ADDRSTRLEN];
        assert(inet_ntop(AF_INET,&(addr_.sin_addr),ip,sizeof(ip))!=NULL&&"IP地址转换可读的字符串失败");
        return ip;
    }

    sa_family_t get_family()const
    { 
        if(is_ipv6_)
        {
            return AF_INET6;
        }
        else
        {
            return AF_INET;
        }
    }
    socklen_t get_len()const
    {
        if(is_ipv6_)
        {
            return static_cast<socklen_t>(sizeof(sockaddr_in6));
        }
        else
        {
            return static_cast<socklen_t>(sizeof(sockaddr_in));
        }
    }

    
    const char* get_port()const
    {
        uint16_t port;
        if(is_ipv6_)
        {
            port=ntohs(addr6_.sin6_port);
        }
        else
        {
            port=ntohs(addr_.sin_port);
        }
        return std::to_string(port).c_str();
    }
    const char* sockaddrToString()const
    {
        std::string str;
        str+=(get_ip());
        str+=" :";
        str+=(get_port());
        return str.c_str();
    }
    const struct sockaddr* getSockAddr()const
    {
        const void* tmp=nullptr;
        if(is_ipv6_)
        {
            tmp=safe_static_cast<const void*>(&addr6_);
        }
        else
        {
            tmp=safe_static_cast<const void*>(&addr_);
            return safe_static_cast<const struct sockaddr*>(tmp);
        }
        return safe_static_cast<const struct sockaddr*>(tmp);
    }
    struct sockaddr* getSockAddr()
    {
        void* tmp=nullptr;
        if(is_ipv6_)
        {
            tmp=safe_static_cast<void*>(&addr6_);
        }
        else
        {
            tmp=safe_static_cast<void*>(&addr_);
        }
        return safe_static_cast<struct sockaddr*>(tmp);
    }
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in* addr);
    static void fromIpPort(in_addr_t ip,uint16_t port,
        struct sockaddr_in* addr);                         
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in6* addr);        

    static void fromIpPort(in6_addr ip,uint16_t port,
        struct sockaddr_in6* addr);                                             
private:
    union 
    {
      struct sockaddr_in addr_;
      struct sockaddr_in6 addr6_;
    }; 
    bool is_ipv6_;
};    
}    
}

#endif