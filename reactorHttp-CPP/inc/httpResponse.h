#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "buffer.h"

// 常用状态码
enum HttpStatusCode {
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

// 响应头
struct ResponseHeader {
    char key[32];
    char value[128];
};

// 定义一个函数指针, 用来组织要回复给客户端的数据块
typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);

// http响应结构体
struct HttpResponse {
    // 状态行: 状态码 状态描述 http version
    enum HttpStatusCode statusCode;
    char statusMsg[128];
    char fileName[128];
    // 响应头数组, 键值对
    struct ResponseHeader* headers;
    int headerNum;
    responseBody sendDataFunc;
};

// init
struct HttpResponse* httpResponseInit();

// destroy
void httpResponseDestroy(struct HttpResponse* response);
// 添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value);
// 组织http响应数据, 数据写到sendBuf然后发送给socket
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);


#endif  // _HTTPRESPONSE_H_