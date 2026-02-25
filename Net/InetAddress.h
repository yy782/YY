#ifndef _YY_NET_INETADDRESS_
#define _YY_NET_INETADDRESS_
#include "config.h"
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

    explicit Address(int port,bool loopbackOnly=false,bool ipv6=false):
    Address(static_cast<uint16_t>(port),loopbackOnly,ipv6){}
    explicit Address(uint16_t port,bool loopbackOnly=false,bool ipv6=false);
    Address(const char* ip,int port,bool ipv6=false):
    Address(ip,static_cast<uint16_t>(port),ipv6){}

    Address(const char* ip,uint16_t port,bool ipv6=false);

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
    
    void set_ipv6_addr(const struct sockaddr_in6& addr6){addr6_=addr6; is_ipv6_=true;}
    void set_ipv4_addr(const struct sockaddr_in& addr){addr_=addr; is_ipv6_=false;}

    const std::string get_ip()const;
    const std::string get_port()const;
    sa_family_t get_family()const;
    socklen_t get_len()const;

    

    const std::string sockaddrToString()const;
    const struct sockaddr* getSockAddr()const;
    struct sockaddr* getSockAddr();
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