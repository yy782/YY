#include "../TcpClient.h"
#include "../HTTP/http.h"
#include <functional>
#include <map>
#include <iostream>
using namespace yy;
using namespace yy::net;

class HTTPClient {
public:
    typedef std::function<void(const Http::HttpResponse&)> ResponseCallback;
    typedef std::function<void()> CloseCallback;
    typedef std::function<void()> ConnectedCallback;
    
    HTTPClient(const Address& serverAddr, EventLoop* loop)
        : client_(serverAddr, loop)
         {
        
        client_.setMessageCallBack(std::bind(&HTTPClient::onMessage, this, _1));
        client_.setCloseCallBack([this](TcpConnectionPtr){
        
            closeCb_();
        });
        client_.setConnectedCallback([this](TcpConnectionPtr){
            std::cout << "Connected! Commands: get, post, quit" << std::endl;
            conCb_();
        });
    }
    
    void connect() {
        client_.connect();
    }
    
    void disconnect() {
        client_.disconnect();
    }
    

    
    // GET请求
    void get(const std::string& url, ResponseCallback cb) {
        sendRequest("GET", url, "", cb);
    }
    
    // POST请求
    void post(const std::string& url, const std::string& body, 
              const std::map<std::string, std::string>& headers = {},
              ResponseCallback cb = nullptr) {
        sendRequest("POST", url, body, cb, headers);
    }
    
    // PUT请求
    void put(const std::string& url, const std::string& body,
             const std::map<std::string, std::string>& headers = {},
             ResponseCallback cb = nullptr) {
        sendRequest("PUT", url, body, cb, headers);
    }
    
    // DELETE请求
    void del(const std::string& url, ResponseCallback cb = nullptr) {
        sendRequest("DELETE", url, "", cb);
    }
    
    // 设置连接关闭回调
    void setCloseCallback(CloseCallback cb) {
        closeCb_ = cb;
    }
    void setConCallback(CloseCallback cb) {
        conCb_ = cb;
    }
private:
    struct RequestContext {
        std::string method;
        std::string url;
        std::map<std::string, std::string> headers;
        ResponseCallback callback;
    };
    
    void sendRequest(const std::string& method, const std::string& url, 
                     const std::string& body, ResponseCallback cb,
                     const std::map<std::string, std::string>& headers = {}) 
                     {
        
        // 构建HTTP请求
        Http::HttpRequest req;
        
        // 设置方法
        if (method == "GET") req.method_ = Http::HttpRequest::Method::GET;
        else if (method == "POST") req.method_ = Http::HttpRequest::Method::POST;
        else if (method == "PUT") req.method_ = Http::HttpRequest::Method::PUT;
        else if (method == "DELETE") req.method_ = Http::HttpRequest::Method::DELETE;
        
        req.queryUrl_ = url;
        req.version_ = "HTTP/1.1";
        
        // 添加Host头
        std::string host = client_.getPeerAddr().sockaddrToString();
        req.headers_["Host"] = host.substr(0, host.find(':'));
        
        // 添加用户自定义头
        for (const auto& h : headers) {
            req.headers_[h.first] = h.second;
        }
        
        // 设置body
        if (!body.empty()) {
            req.body_ = body;
            req.headers_["Content-Type"] = "application/x-www-form-urlencoded";
        }
        
        // 编码并发送
        std::string requestData;
        req.encode(requestData);
        client_.send(std::move(requestData));
        
        // 保存上下文
        RequestContext ctx;
        ctx.method = method;
        ctx.url = url;
        ctx.headers = headers;
        ctx.callback = cb;
        
        pendingRequests_.push(ctx);
    }
    
    void onMessage(TcpConnectionPtr conn) {
        TcpBuffer& buffer = conn->getRecvBuffer();
        
        while(true)
        {
            Http::ParseResult result = response_.tryDecode(
                string_view(buffer.peek(), buffer.get_readable_size()),
                true  // copy body
            );
            
            if (result == Http::ParseResult::Error) {
                std::cerr<<"HTTP response parse error"<<std::endl;
                buffer.consume(buffer.get_readable_size());
                return;
            }
            
            if (result == Http::ParseResult::NotComplete) {
                // 数据不完整，等待更多
                return;
            }
            
            if (result == Http::ParseResult::Complete) {
                // 解析完成
                if (!pendingRequests_.empty()) {
                    RequestContext ctx = pendingRequests_.front();
                    pendingRequests_.pop();
                    
                    std::cout<<"HTTP response: " << static_cast<int>(response_.status_) 
                                << " for " << ctx.method << " " << ctx.url<<std::endl;               
                    
                    // 调用回调
                    if (ctx.callback) {
                        ctx.callback(response_);
                    }
                }
                
                // 消费缓冲区中的数据
                buffer.consume(response_.getByte());
                
                // 检查是否保持连接
                std::string connection = response_.getHeader("connection");
                if (connection == "close") {
                    conn->disconnect();
                } else {
                    // 重置响应解析器
                    response_.clear();
                }
            }            
        }

    }
    

    
    TcpClient client_;
    Http::HttpResponse response_;
    std::queue<RequestContext> pendingRequests_;
    ConnectedCallback conCb_;
    CloseCallback closeCb_;
};
int main() {
    EventLoop loop;
    Address serverAddr("127.0.0.1", 8080);
    
    HTTPClient client(serverAddr, &loop);
    client.setConCallback([&](){
            client.get("/user?name=Tom", [](const Http::HttpResponse& resp) {
                    std::cout << "Response: " << resp.body_ << std::endl;
            });
            sleep(1);

            std::map<std::string, std::string> headers;
            headers["Content-Type"] = "application/json";
            client.post("/api/user", "{\"name\":\"Tom\",\"age\":20}", 
                    headers, [](const Http::HttpResponse& resp) {
                    std::cout << "POST Response: " << resp.body_ << std::endl;
            });
            sleep(1);
            client.disconnect();
    });
    
    client.setCloseCallback([&]() {
        std::cout << "Connection closed" << std::endl;
        loop.quit();
        exit(0);
    });
    
    client.connect();
    loop.loop();
    
    return 0;
}