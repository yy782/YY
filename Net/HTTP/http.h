/**
 * @file http.h
 * @brief HTTP协议相关定义
 * 
 * 本文件定义了HTTP协议的核心组件，包括HTTP消息、请求、响应、解析器和路由器等。
 */

#ifndef _YY_NET_HTTP_H_
#define  _YY_NET_HTTP_H_
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include "../../Common/stringPiece.h"
#include <functional>
namespace yy 
{
namespace net 
{

namespace Http
{
/**
 * @brief HTTP解析结果状态
 */
enum class ParseResult 
{
    Error,        ///< 解析错误
    Complete,     ///< 解析完成
    NotComplete,  ///< 数据不完整
    Continue100,  ///< 需要发送100 Continue
};

/**
 * @brief HTTP版本
 */
enum struct Version 
{
    UNKNOWN,    ///< 未知版本
    HTTP_1_0,   ///< HTTP/1.0
    HTTP_1_1    ///< HTTP/1.1
};

/**
 * @brief Connection类型
 */
enum struct Connection 
{
    UNKNOWN,     ///< 未知类型
    keep_alive,  ///< 保持连接
    close        ///< 关闭连接
};


/**
 * @brief HTTP消息基类
 */
class HttpMsg 
{
public:
    /**
     * @brief 构造函数
     */
    HttpMsg();
    /**
     * @brief 析构函数
     */
    virtual ~HttpMsg() {}
    
    /**
     * @brief 编码消息
     * 
     * @param buf 输出缓冲区
     * @return int 编码后的长度
     */
    virtual int encode(std::string& buf)=0;
    /**
     * @brief 尝试解码消息
     * 
     * @param buf 输入缓冲区
     * @param copyBody 是否复制body
     * @return ParseResult 解析结果
     */
    virtual ParseResult tryDecode(stringPiece buf,bool copyBody=true)=0;
    /**
     * @brief 清空消息
     */
    virtual void clear();
    
    /**
     * @brief 获取指定头字段的值
     * 
     * @param name 头字段名称
     * @return std::string 头字段值
     */
    std::string getHeader(const std::string& name);
    /**
     * @brief 获取消息体
     * 
     * @return stringPiece 消息体
     */
    stringPiece getBody();
    /**
     * @brief 获取消息长度
     * 
     * @return size_t 消息长度
     */
    size_t getByte() const;
    
    /**
     * @brief 头字段映射
     */
    std::map<std::string, std::string> headers_;
    /**
     * @brief HTTP版本
     */
    std::string version_;
    /**
     * @brief 消息体
     */
    std::string body_;
    /**
     * @brief 消息体视图
     */
    stringPiece bodyView_;

protected:
    /**
     * @brief 是否已完成解析
     */
    bool complete_;
    /**
     * @brief 内容长度
     */
    size_t contentLen_;
    /**
     * @brief 已扫描长度
     */
    size_t scanned_;
    
    /**
     * @brief 内部解码方法
     * 
     * @param buf 输入缓冲区
     * @param copyBody 是否复制body
     * @param line1 第一行数据
     * @return ParseResult 解析结果
     */
    ParseResult tryDecode_(stringPiece buf,bool copyBody,stringPiece* line1);
};

/**
 * @brief HTTP请求
 */
class HttpRequest : public HttpMsg 
{
public:
    /**
     * @brief 构造函数
     */
    HttpRequest();
    
    /**
     * @brief HTTP方法
     */
    enum struct Method 
    {
        UNKNOWN,  ///< 未知方法
        GET,      ///< GET方法
        POST,     ///< POST方法
        PUT,      ///< PUT方法
        DELETE,   ///< DELETE方法
        HEAD      ///< HEAD方法
    };
    
    /**
     * @brief 请求方法
     */
    Method method_;
    /**
     * @brief 请求URL
     */
    std::string url_;
    /**
     * @brief 查询URL
     */
    std::string queryUrl_;
    /**
     * @brief URL参数
     */
    std::map<std::string,std::string> args_;
    
    /**
     * @brief 获取URL参数
     * 
     * @param name 参数名称
     * @return std::string 参数值
     */
    std::string getArg(const std::string& name);
    /**
     * @brief 将方法转换为字符串
     * 
     * @param m 方法
     * @return std::string 方法字符串
     */
    static std::string methodToString(Method m);
    
    /**
     * @brief 编码请求
     * 
     * @param buf 输出缓冲区
     * @return int 编码后的长度
     */
    virtual int encode(std::string& buf) override;
    /**
     * @brief 尝试解码请求
     * 
     * @param buf 输入缓冲区
     * @param copyBody 是否复制body
     * @return ParseResult 解析结果
     */
    virtual ParseResult tryDecode(stringPiece buf,bool copyBody=true) override;
    /**
     * @brief 清空请求
     */
    virtual void clear() override;

private:
    /**
     * @brief 将字符串转换为方法
     * 
     * @param s 方法字符串
     * @return Method 方法
     */
    static Method stringToMethod(const std::string& s);
    /**
     * @brief 解析URL参数
     */
    void parseUrlParams();
};

/**
 * @brief HTTP响应
 */
class HttpResponse : public HttpMsg 
{
public:
    /**
     * @brief 构造函数
     */
    HttpResponse();
    
    /**
     * @brief HTTP状态码
     */
    enum struct Status 
    {
        UNKNOWN = 0,       ///< 未知状态
        OK = 200,          ///< 200 OK
        BAD_REQUEST = 400, ///< 400 Bad Request
        NOT_FOUND = 404,   ///< 404 Not Found
        INTERNAL_ERROR = 500 ///< 500 Internal Server Error
    };
    
    /**
     * @brief 状态码
     */
    Status status_;
    /**
     * @brief 状态消息
     */
    std::string statusMsg_; 
    
    /**
     * @brief 设置状态
     * 
     * @param st 状态码
     * @param msg 状态消息
     */
    void setStatus(Status st, const std::string& msg="");
    /**
     * @brief 设置404状态
     */
    void setNotFound();
    
    /**
     * @brief 编码响应
     * 
     * @param buf 输出缓冲区
     * @return int 编码后的长度
     */
    virtual int encode(std::string& buf)override;
    /**
     * @brief 尝试解码响应
     * 
     * @param buf 输入缓冲区
     * @param copyBody 是否复制body
     * @return ParseResult 解析结果
     */
    virtual ParseResult tryDecode(stringPiece buf,bool copyBody=true) override;
    /**
     * @brief 清空响应
     */
    virtual void clear() override;

private:
    /**
     * @brief 将状态码转换为字符串
     * 
     * @param s 状态码
     * @return std::string 状态消息
     */
    static std::string statusToString(Status s);
};

/**
 * @brief HTTP解析器类
 */
class HttpParser 
{
public:
    /**
     * @brief 构造函数
     */
    HttpParser();
    
    /**
     * @brief 解析请求
     * 
     * @param data 输入数据
     * @param len 数据长度
     * @param copyBody 是否复制body
     * @return ParseResult 解析结果
     */
    ParseResult parseRequest(const char* data, size_t len, bool copyBody=true);
    /**
     * @brief 构建响应
     * 
     * @param resp 响应对象
     * @return std::string 响应字符串
     */
    std::string buildResponse(HttpResponse& resp);
    
    /**
     * @brief 获取请求对象
     * 
     * @return HttpRequest& 请求对象
     */
    HttpRequest& getRequest();
    /**
     * @brief 获取响应对象
     * 
     * @return HttpResponse& 响应对象
     */
    HttpResponse& getResponse();
    /**
     * @brief 清空解析器
     */
    void clear();

private:
    /**
     * @brief 解析状态
     */
    enum class ParseState 
    {
        REQUEST_LINE,  ///< 请求行
        HEADERS,       ///< 头字段
        BODY,          ///< 消息体
        COMPLETE       ///< 解析完成
    };
    
    /**
     * @brief 请求对象
     */
    HttpRequest req_;
    /**
     * @brief 响应对象
     */
    HttpResponse resp_;
    /**
     * @brief 输入缓冲区
     */
    std::string inputBuffer_;
    /**
     * @brief 解析状态
     */
    ParseState parseState_;
};

/**
 * @brief HTTP处理器类型
 */
typedef std::function<void(HttpRequest&, HttpResponse&)> HttpHandler;

/**
 * @brief HTTP路由器
 */
class HttpRouter 
{
public:
    /**
     * @brief 构造函数
     */
    HttpRouter();
    
    /**
     * @brief 注册路由
     * 
     * @param url URL路径
     * @param method HTTP方法
     * @param handler 处理器
     */
    void route(const std::string& url,HttpRequest::Method method,HttpHandler handler);
    /**
     * @brief 注册GET路由
     * 
     * @param url URL路径
     * @param handler 处理器
     */
    void get(const std::string& url,HttpHandler handler);
    /**
     * @brief 注册POST路由
     * 
     * @param url URL路径
     * @param handler 处理器
     */
    void post(const std::string& url,HttpHandler handler);
    /**
     * @brief 注册PUT路由
     * 
     * @param url URL路径
     * @param handler 处理器
     */
    void put(const std::string& url,HttpHandler handler);
    /**
     * @brief 注册DELETE路由
     * 
     * @param url URL路径
     * @param handler 处理器
     */
    void del(const std::string& url,HttpHandler handler);  // delete是关键字，用del
    /**
     * @brief 注册HEAD路由
     * 
     * @param url URL路径
     * @param handler 处理器
     */
    void head(const std::string& url,HttpHandler handler);
    
    /**
     * @brief 设置默认处理器
     * 
     * @param handler 默认处理器
     */
    void setDefaultHandler(HttpHandler handler);
    
    /**
     * @brief 路由请求
     * 
     * @param req 请求对象
     * @param resp 响应对象
     * @return bool 是否成功路由
     */
    bool routeRequest(HttpRequest& req,HttpResponse& resp) const;
    
    /**
     * @brief 清空路由
     */
    void clear();
    
    /**
     * @brief 获取路由数量
     * 
     * @return size_t 路由数量
     */
    size_t size() const;

private:
    /**
     * @brief 路由条目
     */
    struct RouteEntry 
    {
        std::string url_;         ///< URL路径
        HttpRequest::Method method_; ///< HTTP方法
        HttpHandler handler_;     ///< 处理器
    };
    
    /**
     * @brief 路由列表
     */
    std::vector<RouteEntry> routes_;
    /**
     * @brief 默认处理器
     */
    HttpHandler defaultHandler_;
    
    /**
     * @brief 默认404处理器
     * 
     * @param req 请求对象
     * @param resp 响应对象
     */
    static void defaultNotFoundHandler(HttpRequest& req,HttpResponse& resp);
};

}
}
}
#endif