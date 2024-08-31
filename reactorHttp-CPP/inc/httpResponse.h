#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "buffer.h"
#include <map>
#include <functional>

// 常用状态码
enum class HttpStatusCode {
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

// http响应结构体
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    // 添加响应头
    void addHeader(const std::string key, const std::string value);
    // 组织http响应数据, 数据写到sendBuf然后发送给socket
    void prepareMsg(Buffer* sendBuf, int socket);

    inline void setFileName(std::string name) { m_fileName = name; }
    inline void setStatusCode(HttpStatusCode code) { m_statusCode = code; }

    std::function<void(const std::string, Buffer*, int)> m_sendDataFunc;

private:
    // 状态行: 状态码 状态描述 http version
    HttpStatusCode m_statusCode;
    std::string m_fileName;
    // 响应头数组, 键值对
    std::map<std::string, std::string> m_headers;
    // 定义状态码和描述的对应关系
    const std::map<int, std::string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"}};
};

#endif  // _HTTPRESPONSE_H_