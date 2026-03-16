#include "request.h"


namespace yy 
{
namespace net 
{

std::shared_ptr<struct http_request> http_request_new() {
    auto req=std::make_shared<struct http_request>();
    
    req->version = NULL;
    req->method = NULL;
    req->url = NULL;
    req->current_state = REQUEST_STATUS;
    req->request_headers = NULL;
    req->request_headers_number = 0;
    
    return req;
}


// 重置请求对象
void http_request_reset(std::shared_ptr<struct http_request> http_req) {
    if (http_req == NULL) return;
    
    http_req=nullptr;
    http_req->current_state = REQUEST_STATUS;
}

// 添加请求头
void http_request_add_header(std::shared_ptr<struct http_request> http_req, char *key, char *value) {
    if (http_req == NULL || key == NULL || value == NULL) return;
    
    // 重新分配头部数组内存
    struct request_header *new_headers = (struct request_header *)realloc(
        http_req->request_headers, 
        (http_req->request_headers_number + 1) * sizeof(struct request_header)
    );
    
    if (new_headers == NULL) return;
    
    http_req->request_headers = new_headers;
    
    // 复制key和value
    http_req->request_headers[http_req->request_headers_number].key = strdup(key);
    http_req->request_headers[http_req->request_headers_number].value = strdup(value);
    
    http_req->request_headers_number++;
}

// 获取请求头
char *http_request_get_header(struct http_request *http_req, char *key) {
    if (http_req == NULL || key == NULL) return NULL;
    
    for (int i = 0; i < http_req->request_headers_number; i++) {
        if (strcasecmp(http_req->request_headers[i].key, key) == 0) {
            return http_req->request_headers[i].value;
        }
    }
    
    return NULL;
}

// 获取当前请求状态
enum http_request_state http_request_current_state(struct http_request *http_req) {
    if (http_req == NULL) return REQUEST_DONE;
    return http_req->current_state;
}

// 检查是否应该关闭连接
int http_request_close_connection(struct http_request *http_req) {
    if (http_req == NULL) return 1; // 默认关闭
    
    char *connection = http_request_get_header(http_req, "Connection");
    if (connection != NULL && strcasecmp(connection, "close") == 0) {
        return 1; // 关闭连接
    }
    
    return 0; // 保持连接
}
}
}