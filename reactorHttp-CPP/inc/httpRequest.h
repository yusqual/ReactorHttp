#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "buffer.h"
#include "httpResponse.h"

struct RequestHeader {
    char* key;
    char* value;
};

// 当前的解析状态
enum HttpRequestStat {
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};

// http请求结构体
struct HttpRequest {
    char* method;
    char* url;
    char* version;
    struct RequestHeader* reqHeaders;
    int reqHeadersNum;
    enum HttpRequestStat curState;
};

// init
struct HttpRequest* httpRequestInit();
// 重置
void httpRequestReset(struct HttpRequest* request);
void httpRequestResetEx(struct HttpRequest* req);
void httpRequestDestroy(struct HttpRequest* request);
// 获取处理状态
enum HttpRequestStat httpRequestState(struct HttpRequest* request);
// 添加key value
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
// 根据key 获取value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);

// 解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
// 解析请求头, 该函数仅处理一行
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);

// 处理http请求
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
                      struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// 解析http请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
// 解码字符串
void decodeMsg(char* to, const char* from);

// 根据文件后缀返回http响应头中的content-type
const char* getFileType(const char* name);

void sendFile(const char* file, struct Buffer* sendBuf, int cfd);
void sendDir(const char* dir, struct Buffer* sendBuf, int cfd);

#endif  // _HTTPREQUEST_H_