#ifndef _YY_NET_INETADDRESS_
#define _YY_NET_INETADDRESS_
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "../Common/copyable.h"
#include "../Common/SyncLog.h"
#include "../Common/Types.h"
namespace yy
{
namespace net
{
class Address:public copyable
{
    
public:
    Address();

    explicit Address(int port,bool loopbackOnly=false,bool ipv6=false):Address(static_cast<uint16_t>(port),loopbackOnly,ipv6){}
    explicit Address(uint16_t port,bool loopbackOnly=false,bool ipv6=false);
    Address(const struct sockaddr_storage& peerAddr);
    Address(const char* ip,int port,bool ipv6=false):Address(ip,static_cast<uint16_t>(port),ipv6){}
    Address(const char* ip,uint16_t port,bool ipv6=false);

    Address& operator=(const struct sockaddr_in6& addr6)
    {
        addr6_=addr6;
        return *this;
    }
    Address& operator=(const struct sockaddr_in& addr)
    {
        addr_=addr;
        return *this;
    }
    Address(const Address& addr);
    Address& operator=(const Address& addr);
    
    void set_ipv6_addr(const struct sockaddr_in6& addr6){addr6_=addr6; is_ipv6_=true;}
    void set_ipv4_addr(const struct sockaddr_in& addr){addr_=addr; is_ipv6_=false;}

    const std::string ip()const;
    const std::string port()const;
    sa_family_t family()const;
    socklen_t len()const;

    

    const std::string sockaddrToString()const;
    const struct sockaddr* sockAddr()const;
    struct sockaddr* sockAddr();
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in* addr);
    static void fromIpPort(in_addr_t ip,uint16_t port,
        struct sockaddr_in* addr);                         
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in6* addr);        

    static void fromIpPort(const struct in6_addr& ip,uint16_t port,
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