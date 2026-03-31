#include "http.h"
#include "../../Common/LogFilter.h"
using namespace yy;
using namespace yy::net;
using namespace yy::net::Http;


HttpMsg::HttpMsg(): 
complete_(false), 
contentLen_(0), 
scanned_(0) 
{
    clear();
}

void HttpMsg::clear() 
{
    headers_.clear();
    version_="HTTP/1.1";
    body_.clear();              
    bodyView_=stringPiece();  
    complete_=false;
    contentLen_=0;
    scanned_=0;
}

std::string HttpMsg::getHeader(const std::string& name) 
{
    auto it=headers_.find(name);
    return it!=headers_.end()?it->second:"";
}

stringPiece HttpMsg::getBody() 
{   
    return bodyView_.size()?bodyView_:stringPiece(body_); 
}

size_t HttpMsg::getByte()const{return scanned_;}

ParseResult HttpMsg::tryDecode_(stringPiece buf,bool copyBody,stringPiece* line1) 
{
    if(complete_)
    {
        return ParseResult::Complete;
    }
    
    // 解析头部
    if(!contentLen_)
    {
        const char* p=buf.data();
        stringPiece headerSlice;
        
        // 查找头部结束标记 \r\n\r\n
        while(buf.size()>=scanned_+4)
        {
            if(p[scanned_]=='\r'&&memcmp(p+scanned_,"\r\n\r\n",4)==0)
            {
                headerSlice=stringPiece(p,p+scanned_);
                break;
            }
            scanned_++;
        }
        
        if(headerSlice.empty())
        {
            return ParseResult::NotComplete;
        }
        
        // 解析请求行
        *line1=headerSlice.eatLine();
        
        // 解析头部字段
        while(headerSlice.size())
        {
            headerSlice.eat(2);  // 跳过 \r\n
            stringPiece line=headerSlice.eatLine();
            stringPiece key=line.eatWord();
            line.trimSpace();
            
            if(key.size()&&line.size()&&key.data()[key.size()-1]==':')
            {
                // 将key转为小写
                std::string keyStr=key.sub(0,-1).toString();
                for(char& c:keyStr)
                {
                    c=static_cast<char>(tolower(static_cast<unsigned char>(c)));
                }
                headers_[keyStr]=line.toString();  
            }else if(key.empty()&&line.empty()&&headerSlice.empty())
            {
                break;
            }else
            {
                LOG_HTTP_DEBUG("[HTTP] 格式错误");
                return ParseResult::Error;
            }
        }
        
        scanned_+=4;  // 跳过 \r\n\r\n
        std::string contentLenStr=getHeader("content-length");
        contentLen_=contentLenStr.empty()?0:static_cast<size_t>(atoll(contentLenStr.c_str()));
        
        // 检查是否需要100-continue
        if(buf.size()<contentLen_+scanned_&&!getHeader("expect").empty())
        {
            return ParseResult::Continue100;
        }
    }
    
    // 解析body
    if(!complete_&&buf.size()>=contentLen_+scanned_)
    {
        if(copyBody)
        {
            body_.assign(buf.data()+scanned_,contentLen_);  
        }else
        {
            bodyView_=stringPiece(buf.data()+scanned_,contentLen_);  
        }
        complete_=true;
        scanned_+=contentLen_;
    }
    
    return complete_?ParseResult::Complete:ParseResult::NotComplete;
}


HttpRequest::HttpRequest():
method_(Method::UNKNOWN)
{
    clear();
}


std::string HttpRequest::getArg(const std::string& name)
{
    auto it=args_.find(name);
    return it!=args_.end()?it->second:"";
}

int HttpRequest::encode(std::string& buf)
{
    size_t oldSize=buf.size();
    char tmp[4096];
    
    // 请求行
    snprintf(tmp,sizeof(tmp),"%s %s %s\r\n",
             methodToString(method_).c_str(),
             queryUrl_.empty()?url_.c_str():queryUrl_.c_str(),
             version_.c_str());
    buf+=tmp;
    
    // 头部
    for(auto& h:headers_)
    {
        buf+=h.first+": "+h.second+"\r\n";
    }
    // 自动添加必要的头部
    buf+="Connection: Keep-Alive\r\n";
    snprintf(tmp,sizeof(tmp),"Content-Length: %zu\r\n",getBody().size());
    buf+=tmp;
    buf+="\r\n";
    
    // body
    stringPiece bodyView=getBody();
    buf.append(bodyView.data(),bodyView.size());
    
    return static_cast<int>(buf.size()-oldSize);
}

ParseResult HttpRequest::tryDecode(stringPiece buf,bool copyBody)
{
    stringPiece line1;
    ParseResult r=tryDecode_(buf,copyBody,&line1);
    
    if(line1.size())
    {
        stringPiece methodView=line1.eatWord();
        stringPiece urlView=line1.eatWord();
        stringPiece verView=line1.eatWord();
        
        method_=stringToMethod(methodView.toString());
        queryUrl_=urlView.toString();
        version_=verView.toString();
        
        // 解析URL中的参数
        parseUrlParams();
    }
    
    return r;
}

void HttpRequest::clear()
{
    HttpMsg::clear();
    method_=Method::UNKNOWN;
    url_.clear();
    queryUrl_.clear();
    args_.clear();
}

std::string HttpRequest::methodToString(Method m)
{
    static std::map<Method,std::string> m2s={
        {Method::GET,"GET"},
        {Method::POST,"POST"},
        {Method::PUT,"PUT"},
        {Method::DELETE,"DELETE"},
        {Method::HEAD,"HEAD"}
    };
    auto it=m2s.find(m);
    return it!=m2s.end()?it->second:"UNKNOWN";
}

HttpRequest::Method HttpRequest::stringToMethod(const std::string& s)
{
    static std::map<std::string,Method> s2m={
        {"GET",Method::GET},
        {"POST",Method::POST},
        {"PUT",Method::PUT},
        {"DELETE",Method::DELETE},
        {"HEAD",Method::HEAD}
    };
    auto it=s2m.find(s);
    return it!=s2m.end()?it->second:Method::UNKNOWN;
}

void HttpRequest::parseUrlParams()
{   
    size_t pos=queryUrl_.find('?');
    if(pos!=std::string::npos){
        url_=queryUrl_.substr(0,pos);
        std::string query=queryUrl_.substr(pos+1);
        
        size_t start=0;
        while(start<query.size()){
            size_t eq=query.find('=',start);
            if(eq==std::string::npos)break;
            
            size_t and_=query.find('&',eq+1);
            if(and_==std::string::npos)and_=query.size();
            
            std::string key=query.substr(start,eq-start);
            std::string value=query.substr(eq+1,and_-eq-1);
            args_[key]=value;
            
            start=and_+1;
        }
    }else{
        url_=queryUrl_;
    }
}

HttpResponse::HttpResponse():status_(Status::OK),statusMsg_("OK")
{
    clear();
}

void HttpResponse::setStatus(Status st,const std::string& msg)
{
    status_=st;
    statusMsg_=msg.empty()?statusToString(st):msg;
}

void HttpResponse::setNotFound()
{
    setStatus(Status::NOT_FOUND,"Not Found");
    body_="404 Not Found";
}

int HttpResponse::encode(std::string& buf)
{
    size_t oldSize=buf.size();
    char tmp[4096];
    
    // 状态行
    snprintf(tmp,sizeof(tmp),"%s %d %s\r\n",
             version_.c_str(),static_cast<int>(status_),statusMsg_.c_str());
    buf+=tmp;
    
    // 头部
    for(auto& h:headers_){
        buf+=h.first+": "+h.second+"\r\n";
    }
    
    // 自动添加必要的头部
    buf+="Connection: Keep-Alive\r\n";
    snprintf(tmp,sizeof(tmp),"Content-Length: %zu\r\n",getBody().size());
    buf+=tmp;
    buf+="\r\n";
    
    // body
    stringPiece bodyView=getBody();
    buf.append(bodyView.data(),bodyView.size());
    
    return static_cast<int>(buf.size()-oldSize);
}

ParseResult HttpResponse::tryDecode(stringPiece buf,bool copyBody)
{
    stringPiece line1;
    ParseResult r=tryDecode_(buf,copyBody,&line1);
    
    if(line1.size()){
        version_=line1.eatWord().toString();
        status_=static_cast<Status>(atoi(line1.eatWord().data()));
        statusMsg_=line1.trimSpace().toString();
    }
    
    return r;
}

void HttpResponse::clear()
{
    HttpMsg::clear();
    status_=Status::OK;
    statusMsg_="OK";
}

std::string HttpResponse::statusToString(Status s)
{
    static std::map<Status,std::string> s2s={
        {Status::OK,"OK"},
        {Status::BAD_REQUEST,"Bad Request"},
        {Status::NOT_FOUND,"Not Found"},
        {Status::INTERNAL_ERROR,"Internal Server Error"}
    };
    auto it=s2s.find(s);
    return it!=s2s.end()?it->second:"Unknown";
}

HttpParser::HttpParser():parseState_(ParseState::REQUEST_LINE){}
//格式化parseRequest方法定义，保持代码风格一致性
ParseResult HttpParser::parseRequest(const char* data,size_t len,bool copyBody)
{
    inputBuffer_.append(data,len);
    ParseResult r=req_.tryDecode(stringPiece(inputBuffer_),copyBody);
    
    if(r==ParseResult::Complete)
    {
        inputBuffer_.erase(0,req_.getByte());
    }else if(r==ParseResult::Error)
    {
        inputBuffer_.clear();
    }
    
    return r;
}

std::string HttpParser::buildResponse(HttpResponse& resp)
{
    std::string buf;
    resp.encode(buf);
    return buf;
}

HttpRequest& HttpParser::getRequest(){return req_;}

HttpResponse& HttpParser::getResponse(){return resp_;}

void HttpParser::clear()
{
    req_.clear();
    resp_.clear();
    inputBuffer_.clear();
    parseState_=ParseState::REQUEST_LINE;
}





HttpRouter::HttpRouter():defaultHandler_(defaultNotFoundHandler){}

void HttpRouter::route(const std::string& url,HttpRequest::Method method,HttpHandler handler)
{
    if(!handler){
        LOG_HTTP_DEBUG("[Router] 注册路由失败：handler为空");
        return;
    }
    
    // 检查是否已存在相同路由，如果存在则更新
    for(auto& entry:routes_)
    {
        if(entry.url_==url&&entry.method_==method)
        {
            LOG_HTTP_DEBUG("[Router] 更新路由: "<<
                          HttpRequest::methodToString(method).c_str()<<" "<<url.c_str());
            entry.handler_=handler;
            return;
        }
    }
    
    // 添加新路由 - 修正：使用参数 url 和 method，而不是成员变量
    LOG_HTTP_DEBUG("[Router] 注册路由: "<<
                   HttpRequest::methodToString(method).c_str()<<" "<<url.c_str());
    routes_.push_back({url,method,handler});  // 修正：使用参数
}

void HttpRouter::get(const std::string& url,HttpHandler handler)
{
    route(url,HttpRequest::Method::GET,handler);
}

void HttpRouter::post(const std::string& url,HttpHandler handler)
{
    route(url,HttpRequest::Method::POST,handler);
}

void HttpRouter::put(const std::string& url,HttpHandler handler)
{
    route(url,HttpRequest::Method::PUT,handler);
}

void HttpRouter::del(const std::string& url,HttpHandler handler)
{
    route(url,HttpRequest::Method::DELETE,handler);
}

void HttpRouter::head(const std::string& url,HttpHandler handler)
{
    route(url,HttpRequest::Method::HEAD,handler);
}

void HttpRouter::setDefaultHandler(HttpHandler handler)
{
    if(handler)
    {
        defaultHandler_=handler;
        LOG_HTTP_DEBUG("[Router] 设置默认处理器");
    }
}

bool HttpRouter::routeRequest(HttpRequest& req,HttpResponse& resp)const
{
    // 查找精确匹配的路由
    for(const auto& entry:routes_)
    {
        if(entry.url_==req.url_&&entry.method_==req.method_)
        {
            LOG_HTTP_DEBUG("[Router] 匹配路由: "<<
                          HttpRequest::methodToString(req.method_).c_str()<<" "<<req.url_.c_str());
            entry.handler_(req,resp);
            return true;
        }
    }
    
    // 未找到路由，使用默认处理器
    defaultHandler_(req,resp);
    return false;
}

void HttpRouter::clear()
{
    routes_.clear();
    LOG_HTTP_DEBUG("[Router] 清空所有路由");
}

size_t HttpRouter::size()const
{
    return routes_.size();
}

void HttpRouter::defaultNotFoundHandler(HttpRequest& req,HttpResponse& resp)
{
    LOG_HTTP_DEBUG("[Router] 404 Not Found: "<<
                   HttpRequest::methodToString(req.method_).c_str()<<" "<<
                   req.url_.c_str());
    resp.setStatus(HttpResponse::Status::NOT_FOUND,"Not Found");
    resp.body_="404 Not Found";
    resp.headers_["Content-Type"]="text/plain";  // 修正：headers_ 加下划线
}