#include "InetAddress.h"

namespace yy
{
namespace net
{
Address::Address():
Address(0,false,false)
{
    
}    
Address::Address(uint16_t port,bool loopbackOnly,bool ipv6):
is_ipv6_(ipv6)
{
    
    if(ipv6)
    {
        struct in6_addr ipv6_addr=loopbackOnly?in6addr_loopback : in6addr_any;
        Address::fromIpPort(ipv6_addr,port,&addr6_);
    }
    else
    {
        in_addr_t ipv4_addr= loopbackOnly ? INADDR_LOOPBACK:INADDR_ANY;
        // 对于 INADDR_LOOPBACK 和 INADDR_ANY，需要转换为网络字节序
        Address::fromIpPort(::htonl(ipv4_addr),port,&addr_);
    }
}
Address::Address(const char* ip,uint16_t port,bool ipv6):
is_ipv6_(ipv6)
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
Address::Address(const struct sockaddr_storage& peerAddr)
{
    if(peerAddr.ss_family==AF_INET)
    {
        is_ipv6_=false;
        memcpy(&addr_,&peerAddr,sizeof(sockaddr_in));
    }
    else
    {
        is_ipv6_=true;
        memcpy(&addr6_,&peerAddr,sizeof(sockaddr_in6));
    }
}
Address::Address(const Address& addr):
is_ipv6_(addr.is_ipv6_)
{
    if(is_ipv6_)
    {
        memcpy(&addr6_,&addr.addr6_,sizeof(sockaddr_in6));
    }
    else
    {
        memcpy(&addr_,&addr.addr_,sizeof(sockaddr_in));
    }
}
 Address& Address::operator=(const Address& addr)
 {
    if(this!=&addr)
    {
        is_ipv6_=addr.is_ipv6_;
        if(is_ipv6_)
        {
            memcpy(&addr6_,&addr.addr6_,sizeof(sockaddr_in6));
        }
        else
        {
            memcpy(&addr_,&addr.addr_,sizeof(sockaddr_in));
        }
    }
    return *this;
 }
 
void Address::fromIpPort(const char* ip, uint16_t port,
    struct sockaddr_in* addr)
{
if(ip==nullptr)
{
    LOG_WARN("空指针访问");
    return;
}
auto r=::inet_addr(ip);
if(r==INADDR_NONE)
{
    LOG_WARN("地址解析失败");
    return;
}
// @learn inet_addr的返回值>=0;=0时地址是0,0,0,0,传入空指针会奔溃
// inet_addr返回的已经是网络字节序，不需要再转换
return fromIpPort(r,port,addr);
}
void Address::fromIpPort(in_addr_t ip,uint16_t port,
    struct sockaddr_in* addr)
{
    if(addr==nullptr)
    {
        LOG_WARN("空指针访问");
        return;
    }
    memZero(addr,sizeof *addr);
    addr->sin_family=AF_INET;
    addr->sin_port=::htons(port);
    addr->sin_addr.s_addr=ip;  // ip 已经是网络字节序
} 
void Address::fromIpPort(const char* ip, uint16_t port,
    struct sockaddr_in6* addr)
{
struct in6_addr ipv6_addr;
auto r=::inet_pton(AF_INET6,ip,&ipv6_addr);
if(r<=0)
{
    if(r<0)
    {
        LOG_WARN("地址转换失败,地址非法");//可能为空指针                                    
    }
    else
    {
        LOG_WARN("地址转换失败，不是IPV6地址");
    }
    return;

}
return fromIpPort(ipv6_addr,port,addr);
}
void Address::fromIpPort(const struct in6_addr& ip, uint16_t port,
    struct sockaddr_in6* addr)
{
    if(addr==nullptr)
    {
        LOG_WARN("空指针访问");
        return;
    }
    memZero(addr,sizeof *addr);
    addr->sin6_family=AF_INET6;
    addr->sin6_port=::htons(port);
    addr->sin6_addr=ip;
}

const std::string Address::get_ip()const
{
    char ip[INET6_ADDRSTRLEN];
    const char* result;
    if(is_ipv6_)
    {
        result = inet_ntop(AF_INET6,&(addr6_.sin6_addr),ip,INET6_ADDRSTRLEN);
    }
    else
    {
        result = inet_ntop(AF_INET,&(addr_.sin_addr),ip,INET_ADDRSTRLEN);
    }
    assert(result!=NULL&&"IP地址转换可读的字符串失败");
    return std::string(ip);
}
const std::string Address::get_port()const
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
    return std::to_string(port);
}
sa_family_t Address::get_family()const
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
socklen_t Address::get_len()const
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
const std::string Address::sockaddrToString()const
{
    std::string str="IP: ";
    str+=(get_ip());
    str+=" Port: ";
    str+=(get_port());
    return str;
}
const struct sockaddr* Address::getSockAddr()const
{
    const void* tmp=nullptr;
    if(is_ipv6_)
    {
        tmp=static_cast<const void*>(&addr6_);
    }
    else
    {
        tmp=static_cast<const void*>(&addr_);
        return static_cast<const struct sockaddr*>(tmp);
    }
    return static_cast<const struct sockaddr*>(tmp);
}
struct sockaddr* Address::getSockAddr()
{
    void* tmp=nullptr;
    if(is_ipv6_)
    {
        tmp=static_cast<void*>(&addr6_);
    }
    else
    {
        tmp=static_cast<void*>(&addr_);
    }
    return static_cast<struct sockaddr*>(tmp);
}
}    
}