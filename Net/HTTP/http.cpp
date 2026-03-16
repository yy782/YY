#include "http.h"
#include "../../Common/LogFilter.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::Http;
int http_request(const char* buf,char* send,int n);//对外开放的唯一API,buf为客户数据，send是要发送的数据，n是send的数组大小

static HttpRequest::Method get_method(const char* method);//将method转换成枚举类型
static Version get_version(const char* version);//将version转换成枚举类型
static const char* get_url(const char* url);//获取url
static Connection get_conn(const char* conn);////将Connection转换成枚举类型
static int parse_http_request(const char* buf,HttpRequest* req);//解析http请求,填充req结构体

static void handle_http_route(HttpRequest& req,HttpResponse& res);//根据HTTP请求，处理回应的HTTP回应
static void handle_index(HttpRequest& req,HttpResponse& res);//处理函数
static void handle_login(HttpRequest& req,HttpResponse& res);//处理函数

static const char* set_version(Version v);
static const char* set_status(HttpResponse::Status s);
static const char* set_conn(Connection c);
static const char* set_len(int len);
static const char* http_msg(HttpResponse& res,size_t n);//将res结构体数据转换为字符串，返回HTTP回应的字符串

int http_request(const char* buf,char* send,int n){
    HttpRequest q{};
    int a=parse_http_request(buf,&q);
    if(a==-1){
        LOG_HTTP_DEBUG("[HTTP] HTTP不支持");
        return a;
    }
    HttpResponse p{};
    handle_http_route(q,p);
    const char* msg=http_msg(p,n);
    //LOG_HTTP_DEBUG("[HTTP] msg\n"<<msg);
    strcpy(send, msg);
    return strlen(send);
}
static HttpRequest::Method get_method(const char* method){//哈希表默认按 指针地址 哈希，而不是按 字符串内容 哈希，所以find()方法无效
    static std::unordered_map<std::string,HttpRequest::Method> m={
        {"GET",HttpRequest::Method::GET},
        {"POST",HttpRequest::Method::POST},
        {"PUT",HttpRequest::Method::PUT}
    };
    auto iter = m.find(std::string(method));
    return iter != m.end() ? iter->second : HttpRequest::Method::UNKNOWN; 
}

static Version get_version(const char* version){
    static std::unordered_map<std::string,Version> m={
      {"HTTP/1.0",Version::HTTP_1_0},
      {"HTTP/1.1",Version::HTTP_1_1}  
    };
    auto iter = m.find(std::string(version));
    return iter != m.end() ? iter->second : Version::UNKNOWN;    
}

static const char* get_url(const char* url){
    return url;
}

static Connection get_conn(const char* conn){
    static std::unordered_map<std::string,Connection> m={
        {"keep-alive",Connection::keep_alive},
        {"close",Connection::close}
    };
    auto iter = m.find(std::string(conn));
    return iter != m.end() ? iter->second : Connection::UNKNOWN;    
}

static int parse_http_request(const char* buf, HttpRequest* req) {
    if (buf==nullptr||req==nullptr) {
        return -1;
    }
    char* saveptr=const_cast<char*>(buf);
    char* method=strtok_r(const_cast<char*>(buf)," ",&saveptr);
    if(!method){
        return -1;
    }
    char* url=strtok_r(nullptr, " ",&saveptr);
    if(!url){
        return -1;
    }
    
    char* version=strtok_r(nullptr,"\r\n",&saveptr);
    if(!version){
        return -1;
    }

    req->method=get_method(method);
    req->version=get_version(version);
    req->url=get_url(url);

    char* line=nullptr;
    while((line=strtok_r(nullptr,"\r\n",&saveptr))!=nullptr) {
        if(strlen(line)==0){
            break;
        }
        char* saveline=line;
        char* key=strtok_r(line,":",&saveline);
        if(!key)continue;
        while(*saveline==' ')++saveline;
        char* value=saveline;
        if(!value)continue;
        if(strcmp(key, "Host")==0){
            LOG_HTTP_DEBUG("[HTTP] Host "<<value);
            req->host=value;
        }else if(strcmp(key,"Content-Length")==0){
            LOG_HTTP_DEBUG("[HTTP] Content-Length "<<value);
            req->content_length=atoi(value);
        } else if(strcmp(key,"Connection")==0){
            LOG_HTTP_DEBUG("[HTTP] Connection "<<value);
            req->conn=get_conn(value);
        }
    }
    req->body=saveptr?saveptr:nullptr;
    return 1;
}

static void handle_http_route(HttpRequest& req,HttpResponse& res){
    static const HttpRoute route_table[]={
        {"/",HttpRequest::Method::GET,handle_index},
        {"/login",HttpRequest::Method::POST,handle_login}
    };
    static const int route_table_size=sizeof(route_table)/sizeof(HttpRoute);
    for(int i=0;i<route_table_size;++i){
        const HttpRoute& rt=route_table[i];
        if(strcmp(req.url,rt.url)==0&&req.method==rt.method){
            rt.handler(req,res);
            return;
        }
    }
    LOG_HTTP_DEBUG("[HTTP] 404 NOT FOUND");
}

static void handle_index(HttpRequest& req,HttpResponse& res){
    res.conn=req.conn;
    res.version=req.version;
    res.body="Hello HTTP World";
    res.status=HttpResponse::Status::OK;
    res.content_length=strlen(res.body);
}

static void handle_login(HttpRequest& req,HttpResponse& res){
    res.conn=req.conn;
    res.version=req.version;
    res.body="该功能未开放";
    res.status=HttpResponse::Status::OK;
    res.content_length=strlen(res.body);
}

static const char* set_version(Version v){
    static std::unordered_map<Version,std::string> m={
        {Version::HTTP_1_0,"HTTP/1.0"},
        {Version::HTTP_1_1,"HTTP/1.1"}
    };
    auto iter=m.find(v);
    return iter!=m.end()?iter->second.data():"UNKNOWN";
}
static const char* set_status(HttpResponse::Status s){
    static std::unordered_map<HttpResponse::Status,std::string> m={
        {HttpResponse::Status::OK,"200 OK"},
        {HttpResponse::Status::NOT_FOUND,"404 NOT_FOUND"}
    };
    auto iter=m.find(s);
    return iter!=m.end()?iter->second.data():"UNKNOWN";    
}
static const char* set_conn(Connection c){
    static std::unordered_map<Connection,std::string> m={
        {Connection::keep_alive,"Conntecion: keep-alive"},
        {Connection::close,"Conntecion: close"}
    };
    auto iter=m.find(c);
    return iter!=m.end()?iter->second.data():"UNKNOWN";   
}
static const char* set_len(int len){
    static std::string str;//static 解决局部变量销毁的野指针问题
    str="Content-Length: "+std::to_string(len);
    return str.data();
}
static const char* http_msg(HttpResponse& res,size_t n){
    static std::string resp;
    resp.clear();
    const char* version=set_version(res.version);
    const char* status=set_status(res.status);
    const char* conn=set_conn(res.conn);
    const char* len=set_len(res.content_length);
    if(strlen(version)+strlen(status)+strlen(conn)+strlen(len)+strlen(res.body)>n)return "缓冲区不足";
    resp+=version;
    resp+=" ";
    resp+=status;
    resp+="\r\n";
    resp+=conn;
    resp+="\r\n";
    resp+=len;
    resp+="\r\n";
    resp+="\r\n";
    resp+=res.body;
    return resp.c_str();
}