#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace yy 
{
namespace net 
{

std::shared_ptr<struct http_response> http_response_new() {
    auto resp=std::make_shared<struct http_response>();
    
    resp->status_code = Unknown;
    resp->status_message = NULL;
    resp->content_type = NULL;
    resp->body = NULL;
    resp->response_headers = NULL;
    resp->response_headers_number = 0;
    resp->keep_connected = 1;
    
    return resp;
}

// HTTP响应编码到缓冲区
void http_response_encode_buffer(std::shared_ptr<struct http_response> http_response,Buffer* buffer)
 {
    if (http_response == NULL || buffer == NULL) {
        return;
    }
    
    char temp[4096];
    int offset = 0;
    
    // 写入状态行
    const char *status_msg = http_response->status_message;
    if (status_msg == NULL) {
        switch (http_response->status_code) {
            case OK: status_msg = "OK"; break;
            case MovedPermanently: status_msg = "Moved Permanently"; break;
            case BadRequest: status_msg = "Bad Request"; break;
            case NotFound: status_msg = "Not Found"; break;
            default: status_msg = "Unknown"; break;
        }
    }
    
    offset += snprintf(temp + offset, sizeof(temp) - offset, 
                       "HTTP/1.1 %d %s\r\n", 
                       http_response->status_code, status_msg);
    
    // 写入Content-Type
    if (http_response->content_type != NULL) {
        offset += snprintf(temp + offset, sizeof(temp) - offset,
                          "Content-Type: %s\r\n", http_response->content_type);
    }
    
    // 写入Content-Length
    int body_len = http_response->body ? strlen(http_response->body) : 0;
    offset += snprintf(temp + offset, sizeof(temp) - offset,
                      "Content-Length: %d\r\n", body_len);
    
    // 写入Connection头
    offset += snprintf(temp + offset, sizeof(temp) - offset,
                      "Connection: %s\r\n", 
                      http_response->keep_connected ? "keep-alive" : "close");
    
    // 写入其他响应头
    for (int i = 0; i < http_response->response_headers_number; i++) {
        offset += snprintf(temp + offset, sizeof(temp) - offset,
                          "%s: %s\r\n",
                          http_response->response_headers[i].key,
                          http_response->response_headers[i].value);
    }
    
    // 空行分隔头部和主体
    offset += snprintf(temp + offset, sizeof(temp) - offset, "\r\n");
    
    // 写入缓冲区
   
    buffer->append(temp, offset);
    
    // 写入响应体
    if (http_response->body != NULL && body_len > 0) {
        buffer->append(http_response->body, body_len);
    }
}
}
}
