#include "httpResponse.h"
#include <strings.h>

const int res_header_size = 16;

struct HttpResponse* httpResponseInit() {
    struct HttpResponse* response = (struct HttpResponse*) malloc(sizeof(struct HttpResponse));
    errif_exit(response == NULL, "httpResponseInit_1", true);
    response->headerNum = 0;
    response->headers = (struct ResponseHeader*) calloc(res_header_size, sizeof(struct ResponseHeader*));
    errif_exit(response->headers == NULL, "httpResponseInit_2", true);
    response->statusCode = Unknown;
    bzero(response->statusMsg, sizeof(response->statusMsg));
    bzero(response->fileName, sizeof(response->fileName));
    // 函数指针
    response->sendDataFunc = NULL;
    return response;
}

void httpResponseDestroy(struct HttpResponse* response) {
    if (response) {
        free(response->headers);
        free(response);
    }
}

void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value) {
    if (response == NULL || key == NULL || value == NULL) return;
    strcpy(response->headers[response->headerNum].key, key);
    strcpy(response->headers[response->headerNum].value, value);
    ++response->headerNum;
}

void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket) {
    // 状态行
    char tmp[1024] = {0};
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
    bufferAppendString(sendBuf, tmp);
    // 响应头
    for (int i = 0; i < response->headerNum; ++i) {
        bzero(tmp, sizeof(tmp));
        sprintf(tmp, "%s: %s", response->headers[i].key, response->headers[i].value);
        bufferAppendString(sendBuf, tmp);
    }
    // 空行
    bufferAppendString(sendBuf, "\r\n");
    // 回复的数据
    response->sendDataFunc(response->fileName, sendBuf, socket);
}
