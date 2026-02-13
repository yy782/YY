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
    explicit Address(uint16_t port,bool loopbackOnly=false,bool ipv6=false)
    {
        if(ipv6)
        {   
            struct in6_addr ipv6=loopbackOnly?in6addr_loopback : in6addr_any;
            Address::fromIpPort(ipv6,port,&addr6_);
        }
        else
        {
            
            auto ipv4= loopbackOnly ? INADDR_LOOPBACK:INADDR_ANY;
            Address::fromIpPort(ipv4,port,&addr_);
        }
    }
    Address& operator=(const struct sockaddr_in6& addr){addr6_=addr;}
    Address& operator=(const struct sockaddr_in& addr){addr_=addr;}
    Address(const char* ip,uint16_t port,bool ipv6=false)
    {
        if(ipv6)
        {
            Address::fromIpPort(ip,port,&addr6_);
        }
        else
        {
            Address::fromIpPort(ip,port,&addr_);
        }
    }
    const char* get_ip()const
    {
        char ip[INET_ADDRSTRLEN];
        assert(inet_ntop(AF_INET,&(addr_.sin_addr),ip,sizeof(ip))!=NULL&&"IP地址转换可读的字符串失败");
        return ip;
    }
    sa_family_t family() const { return addr_.sin_family; }
    // @brief 这个family可以判断对象存储的是ipv6还是ipv4
    socklen_t get_len()const{return static_cast<socklen_t>(sizeof(struct sockaddr_in6));}

    
    const char* get_port()const
    {
        auto port=ntohs(addr_.sin_port);

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
        return safe_static_cast<const struct sockaddr*>(&addr_);
    }
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in* addr)
    {
    if(ip==nullptr)
    {
        LOG_NULL_WARN("空指针访问");
        return;
    }
    auto r=::inet_addr(ip);
    if(r==INADDR_NONE)
    {
        LOG_NULL_WARN("地址解析失败");
        return;
    }
    // @learn inet_addr的返回值>=0;=0时地址是0,0,0,0,传入空指针会奔溃
    return fromInPort(r,port,addr);
    }
    static void fromInPort(in_addr_t ip,uint16_t port,
        struct sockaddr_in* addr)
    {
        if(addr==nullptr)
        {
            LOG_NULL_WARN("空指针访问");
            return;
        }
        memZero(addr,sizeof *addr);
        addr->sin_family=AF_INET;
        addr->sin_port=::htons(port);
        addr->sin_addr.s_addr=::htonl(ip);
    }                         
    static void fromInPort(const char* ip, uint16_t port,
        struct sockaddr_in6* addr)
    {
    struct in6_addr ipv6_addr;
    auto r=::inet_pton(AF_INET6,ip,&ipv6_addr);
    if(r<=0)
    {
        if(r<0)
        {
            LOG_NULL_WARN("地址转换失败,地址非法");//可能为空指针                                    
        }
        else
        {
            LOG_NULL_WARN("地址转换失败，不是IPV6地址");
        }
        return;

    }
    return fromInPort(ipv6_addr,port,addr);
    }        

    static void fromInPort(in6_addr ip,uint16_t port,
        struct sockaddr_in6* addr)
    {
        if(addr==nullptr)
        {
            LOG_NULL_WARN("空指针访问");
            return;
        }
        memZero(addr,sizeof *addr);
        addr->sin6_family=AF_INET6;
        addr->sin6_port=::htons(port);
        addr->sin6_addr=ip;                           
    }                                             
private:
    union {
      struct sockaddr_in addr_;
      struct sockaddr_in6 addr6_;
    }; 
};    
}    
}

#endif