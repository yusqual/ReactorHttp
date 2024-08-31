#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "buffer.h"
#include "httpResponse.h"
#include <map>
#include <functional>

// 当前的解析状态
enum class ProcessStatus : char {
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};

// http请求结构体
class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    // 重置
    void reset();
    // 获取处理状态
    inline ProcessStatus getStatus() { return m_curState; }
    // 添加key value
    void addHeader(const std::string key, const std::string value);
    // 根据key 获取value
    std::string getHeader(const std::string key);

    // 解析请求行
    bool parseRequestLine(Buffer* readBuf);
    // 解析请求头, 该函数仅处理一行
    bool parseRequestHeader(Buffer* readBuf);

    // 处理http请求
    bool parseHttpRequest(Buffer* readBuf,
                          HttpResponse* response, Buffer* sendBuf, int socket);
    // 解析http请求协议
    bool processHttpRequest(HttpResponse* response);

    static void sendFile(const std::string file, Buffer* sendBuf, int cfd);
    static void sendDir(const std::string dir, Buffer* sendBuf, int cfd);

    inline void setMethod(std::string method) { m_method = method; }
    inline void setUrl(std::string url) { m_url = url; }
    inline void setVersion(std::string version) { m_version = version; }
    inline void setStatus(ProcessStatus status) { m_curState = status; }

private:
    // 解码字符串
    std::string decodeMsg(const std::string msg);
    int hexToDec(char c);

    // 根据文件后缀返回http响应头中的content-type
    const std::string getFileType(const std::string name);

    char* splitRequestLine(const char* start, const char* end, const char* sub, std::function<void(std::string)> callback);

private:
    std::string m_method;
    std::string m_url;
    std::string m_version;
    std::map<std::string, std::string> m_reqHeaders;
    enum ProcessStatus m_curState;
};

#endif  // _HTTPREQUEST_H_