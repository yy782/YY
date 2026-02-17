#include "InetAddress.h"

namespace yy
{
namespace net
{
Address::Address(uint16_t port,bool loopbackOnly,bool ipv6)
{
    if(ipv6)
    {   
        struct in6_addr ipv6_addr=loopbackOnly?in6addr_loopback : in6addr_any;
        Address::fromIpPort(ipv6_addr,port,&addr6_);
    }
    else
    {
        in_addr_t ipv4_addr= loopbackOnly ? INADDR_LOOPBACK:INADDR_ANY;
        Address::fromIpPort(ipv4_addr,port,&addr_);
    }
}
Address::Address(const char* ip,uint16_t port,bool ipv6)
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
void Address::fromIpPort(const char* ip, uint16_t port,
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
return fromIpPort(r,port,addr);
}
void Address::fromIpPort(in_addr_t ip,uint16_t port,
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
void Address::fromIpPort(const char* ip, uint16_t port,
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
return fromIpPort(ipv6_addr,port,addr);
}
void Address::fromIpPort(in6_addr ip,uint16_t port,
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
}    
}