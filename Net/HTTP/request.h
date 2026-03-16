#ifndef _YY_NET_HTTP_REQUEST_H
#define _YY_NET_HTTP_REQUEST_H
#include <memory>
struct request_header 
{
    char *key;
    char *value;
};

enum http_request_state 
{
    REQUEST_STATUS,     // wait parse state
    REQUEST_HEADERS,    // wait parse header
    REQUEST_BODY,       // wait parse body
    REQUEST_DONE        // parse done
};

struct http_request {
    char *version;
    char *method;
    char *url;
    enum http_request_state current_state;
    struct request_header *request_headers;
    int request_headers_number;
};

// init request obj

std::shared_ptr<struct http_request> http_request_new();



void http_request_reset(std::shared_ptr<struct http_request> http_req);

void http_request_add_header(std::shared_ptr<struct http_request> http_req, char *key, char *value);

char *http_request_get_header(std::shared_ptr<struct http_request> http_req, char *key);

enum http_request_state http_request_current_state(std::shared_ptr<struct http_request> http_req);

int http_request_close_connection(std::shared_ptr<struct http_request> http_req);

#endif