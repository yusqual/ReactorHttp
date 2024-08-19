#include "httpRequest.h"
#include <strings.h>
#include <sys/stat.h>

const int HeaderSize = 12;

struct HttpRequest* httpRequestInit() {
    struct HttpRequest* request = (struct HttpRequest*) malloc(sizeof(struct HttpRequest));
    errif_exit(request == NULL, "httpRequestInit", true);
    request->reqHeaders = NULL;
    httpRequestReset(request);
    request->reqHeaders = (struct HttpRequest*) malloc(sizeof(struct HttpRequest) * HeaderSize);
    return request;
}

void httpRequestReset(struct HttpRequest* request) {
    request->curState = ParseReqLine;
    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->version) free(request->version);
    if (request->reqHeaders) {
        for (int i = 0; i < request->reqHeadersNum; ++i) {
            free(request->reqHeaders[i].key);
            free(request->reqHeaders[i].value);
        }
        free(request->reqHeaders);
    }
    request->method = NULL;
    request->url = NULL;
    request->version = NULL;
    request->reqHeadersNum = 0;
}

void httpRequestDestroy(struct HttpRequest* request) {
    if (request != NULL) {
        httpRequestReset(request);
        free(request);
    }
}

enum HttpRequestStat httpRequestState(struct HttpRequest* request) {
    return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value) {
    request->reqHeaders[request->reqHeadersNum].key = key;
    request->reqHeaders[request->reqHeadersNum].value = value;
    ++request->reqHeadersNum;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char* key) {
    if (request) {
        for (int i = 0; i < request->reqHeadersNum; ++i) {
            if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0) {  // 不区分大小写比较strlen(key)个字符是否相等
                return request->reqHeaders[i].value;
            }
        }
    }
    return NULL;
}

char* splitRequestLine(const char* start, const char* end, const char* sub, char** ptr) {
    char* space = end;
    if (sub) {
        space = memmem(start, end - start, sub, strlen(sub));
        errif_exit(space == NULL, "splitRequestLine_1", true);
    }
    int length = space - start;  // 请求方式长度 Get Post...
    char* tmp = (char*) malloc(length + 1);
    errif_exit(tmp == NULL, "parseHttpRequestLine_2", true);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}

bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf) {
    // 读出请求行, 保存字符串起始地址
    char* end = bufferFindCRLF(readBuf);
    // 保存字符串结束地址
    char* start = readBuf->data + readBuf->readPos;
    // 请求行总长度
    int lineLen = end - start;
    if (lineLen) {
        // Get /xxx/xx.xx http/x.x
        // 请求方式
        start = splitRequestLine(start, end, " ", &request->method);
        start = splitRequestLine(start, end, " ", &request->url);
        splitRequestLine(start, end, NULL, &request->version);
#if 0
        char* space = memmem(start, lineLen, " ", 1);
        errif_exit(space == NULL, "parseHttpRequestLine_1", true);
        int methodLen = space - start;  // 请求方式长度 Get Post...
        request->method = (char*) malloc(methodLen + 1);
        errif_exit(request->method == NULL, "parseHttpRequestLine_2", true);
        strncpy(request->method, start, methodLen);
        request->method[methodLen] = '\0';
        // 请求的静态资源
        start = space + 1;
        space = memmem(start, end - start, " ", 1);
        errif_exit(space == NULL, "parseHttpRequestLine_3", true);
        int urlLen = space - start;  // 请求方式长度 Get Post...
        request->url = (char*) malloc(urlLen + 1);
        errif_exit(request->url == NULL, "parseHttpRequestLine_4", true);
        strncpy(request->url, start, urlLen);
        request->url[urlLen] = '\0';
        // http版本
        start = space + 1;
        request->version = (char*) malloc(end - start + 1);
        errif_exit(request->version == NULL, "parseHttpRequestLine_4", true);
        strncpy(request->version, start, end - start);
        request->version[end - start] = '\0';
#endif
        // 为解析请求头做准备
        readBuf->readPos += (lineLen + 2);  // 2: "\r\n"

        // 修改状态
        request->curState = ParseReqHeaders;
        return true;
    }
    return false;
}

bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf) {
    char* end = bufferFindCRLF(readBuf);
    if (end) {
        char* start = readBuf->data + readBuf->readPos;
        int lineLen = end - start;
        // 基于:搜索字符串
        char* middle = memmem(start, lineLen, ": ", 2);
        if (middle) {
            int tmpLen = middle - start;  // key长度
            char* key = (char*) malloc(tmpLen + 1);
            errif_exit(key == NULL, "parseHttpRequestHeader_1", true);
            strncpy(key, start, tmpLen);
            key[tmpLen] = '\0';

            tmpLen = end - middle - 2;  // value长度
            char* value = (char*) malloc(tmpLen + 1);
            errif_exit(value == NULL, "parseHttpRequestHeader_2", true);
            strncpy(value, middle + 2, tmpLen);
            key[tmpLen] = '\0';

            httpRequestAddHeader(request, key, value);
            // 移动读数据位置
            readBuf->readPos += (lineLen + 2);
        }
    } else {
        // 请求头解析完毕
        readBuf->readPos += 2;
        // 修改解析状态
        // 忽略POST请求,按照GET设置
        request->curState = ParseReqDone;
    }
    return false;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf) {
    bool result = true;
    while (request->curState != ParseReqDone) {
        switch (request->curState) {
            case ParseReqLine:
                result = parseHttpRequestLine(request, readBuf);
                break;
            case ParseReqHeaders:
                result = parseHttpRequestHeader(request, readBuf);
                break;
            case ParseReqBody:
                printf("POST REQUEST.\n");
                break;
            default:
                break;
        }
    }
    if (result && request->curState == ParseReqDone) {
        // 1. 根据解析出的数据,对客户端的请求做出处理
        // 2.
    }
    request->curState = ParseReqLine;
    return result;
}

// 处理基于GET的http请求
bool processHttpRequest(struct HttpRequest* request) {
    if (strcasecmp(request->method, "get") != 0) {  // strcasecmp 不区分大小写
        return -1;
    }
    decodeMsg(request->url, request->url);
    char* path = (char*)malloc(sizeof(request->url) + 1);
    // 格式化访问的资源路径, 在最前面加上'.'来访问本地目录
    path[0] = '.';
    memcpy(path + 1, request->url, sizeof(request->url));
    free(request->url);
    request->url = path;
    
    printf("method: %s, path: %s\n", request->method, request->url);

    // 获取文件属性, 判断是文件还是目录
    struct stat st;
    int ret = stat(request->url, &st);
    if (ret == -1) {
        // 文件不存在 -> 回复404
        // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        // sendFile("404.html", cfd);
        return 0;
    }
    // 判断是否是目录
    if (S_ISDIR(st.st_mode)) {
        // 目录, 返回目录中的内容
        // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        // sendDir(path, cfd);
    } else {
        // 文件, 返回文件内容
        // sendHeadMsg(cfd, 200, "OK", getFileType(path), st.st_size);
        // sendFile(path, cfd);
    }
    return 0;
}


// 将字符转换为整形数
int hexToDec(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, const char* from) {
    for (; *from != '\0'; ++to, ++from) {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        } else {
            // 字符拷贝, 赋值
            *to = *from;
        }
    }
    *to = '\0';
}