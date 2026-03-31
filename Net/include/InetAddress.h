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
/**
 * @brief 网络地址类，封装了IPv4和IPv6地址
 * 
 * Address类封装了网络地址的处理，支持IPv4和IPv6地址。
 * 它提供了获取和设置IP地址、端口号等功能。
 */
class Address:public copyable
{
    
public:
    /**
     * @brief 默认构造函数
     */
    Address();

    /**
     * @brief 构造函数，使用端口号创建地址
     * 
     * @param port 端口号
     * @param loopbackOnly 是否只使用回环地址
     * @param ipv6 是否使用IPv6地址
     */
    explicit Address(int port,bool loopbackOnly=false,bool ipv6=false):Address(static_cast<uint16_t>(port),loopbackOnly,ipv6){}
    
    /**
     * @brief 构造函数，使用端口号创建地址
     * 
     * @param port 端口号
     * @param loopbackOnly 是否只使用回环地址
     * @param ipv6 是否使用IPv6地址
     */
    explicit Address(uint16_t port,bool loopbackOnly=false,bool ipv6=false);
    
    /**
     * @brief 构造函数，使用sockaddr_storage创建地址
     * 
     * @param peerAddr sockaddr_storage结构
     */
    Address(const struct sockaddr_storage& peerAddr);
    
    /**
     * @brief 构造函数，使用IP地址和端口号创建地址
     * 
     * @param ip IP地址字符串
     * @param port 端口号
     * @param ipv6 是否使用IPv6地址
     */
    Address(const char* ip,int port,bool ipv6=false):Address(ip,static_cast<uint16_t>(port),ipv6){}
    
    /**
     * @brief 构造函数，使用IP地址和端口号创建地址
     * 
     * @param ip IP地址字符串
     * @param port 端口号
     * @param ipv6 是否使用IPv6地址
     */
    Address(const char* ip,uint16_t port,bool ipv6=false);

    /**
     * @brief 赋值运算符，使用sockaddr_in6赋值
     * 
     * @param addr6 sockaddr_in6结构
     * @return Address& 地址对象的引用
     */
    Address& operator=(const struct sockaddr_in6& addr6)
    {
        addr6_=addr6;
        return *this;
    }
    
    /**
     * @brief 赋值运算符，使用sockaddr_in赋值
     * 
     * @param addr sockaddr_in结构
     * @return Address& 地址对象的引用
     */
    Address& operator=(const struct sockaddr_in& addr)
    {
        addr_=addr;
        return *this;
    }
    
    /**
     * @brief 拷贝构造函数
     * 
     * @param addr 地址对象
     */
    Address(const Address& addr);
    
    /**
     * @brief 赋值运算符
     * 
     * @param addr 地址对象
     * @return Address& 地址对象的引用
     */
    Address& operator=(const Address& addr);
    
    /**
     * @brief 设置IPv6地址
     * 
     * @param addr6 sockaddr_in6结构
     */
    void set_ipv6_addr(const struct sockaddr_in6& addr6){addr6_=addr6; is_ipv6_=true;}
    
    /**
     * @brief 设置IPv4地址
     * 
     * @param addr sockaddr_in结构
     */
    void set_ipv4_addr(const struct sockaddr_in& addr){addr_=addr; is_ipv6_=false;}

    /**
     * @brief 获取IP地址
     * 
     * @return const std::string IP地址字符串
     */
    const std::string ip()const noexcept;
    
    /**
     * @brief 获取端口号
     * 
     * @return const std::string 端口号字符串
     */
    const std::string port()const noexcept;
    
    /**
     * @brief 获取地址族
     * 
     * @return sa_family_t 地址族
     */
    sa_family_t family()const noexcept;
    
    /**
     * @brief 获取地址长度
     * 
     * @return socklen_t 地址长度
     */
    socklen_t len()const noexcept;

    /**
     * @brief 将地址转换为字符串
     * 
     * @return const std::string 地址字符串
     */
    const std::string sockaddrToString()const noexcept;
    
    /**
     * @brief 获取sockaddr指针
     * 
     * @return const struct sockaddr* sockaddr指针
     */
    const struct sockaddr* sockAddr()const noexcept;
    
    /**
     * @brief 获取sockaddr指针
     * 
     * @return struct sockaddr* sockaddr指针
     */
    struct sockaddr* sockAddr() noexcept;
    
    /**
     * @brief 从IP地址和端口号创建sockaddr_in结构
     * 
     * @param ip IP地址字符串
     * @param port 端口号
     * @param addr sockaddr_in结构指针
     */
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in* addr);
    
    /**
     * @brief 从IP地址和端口号创建sockaddr_in结构
     * 
     * @param ip IP地址
     * @param port 端口号
     * @param addr sockaddr_in结构指针
     */
    static void fromIpPort(in_addr_t ip,uint16_t port,
        struct sockaddr_in* addr);                          
    
    /**
     * @brief 从IP地址和端口号创建sockaddr_in6结构
     * 
     * @param ip IP地址字符串
     * @param port 端口号
     * @param addr sockaddr_in6结构指针
     */
    static void fromIpPort(const char* ip, uint16_t port,
        struct sockaddr_in6* addr);        

    /**
     * @brief 从IP地址和端口号创建sockaddr_in6结构
     * 
     * @param ip IP地址
     * @param port 端口号
     * @param addr sockaddr_in6结构指针
     */
    static void fromIpPort(const struct in6_addr& ip,uint16_t port,
        struct sockaddr_in6* addr);                                             
private:
    /**
     * @brief 地址联合体
     * 
     * 包含IPv4和IPv6地址结构。
     */
    union 
    {
      /**
       * @brief IPv4地址结构
       */
      struct sockaddr_in addr_;
      /**
       * @brief IPv6地址结构
       */
      struct sockaddr_in6 addr6_;
    }; 
    /**
     * @brief 是否是IPv6地址
     */
    bool is_ipv6_;
};    
}    
}

#endif