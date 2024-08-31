#include "httpResponse.h"
#include <strings.h>
#include "log.h"

HttpResponse::HttpResponse(): m_statusCode(HttpStatusCode::Unknown), m_fileName(std::string()), m_sendDataFunc(nullptr) {
    m_headers.clear();
}

HttpResponse::~HttpResponse() {
}

void HttpResponse::addHeader(const std::string key, const std::string value) {
    if (key.empty() || value.empty()) return;
    m_headers.insert(std::make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer* sendBuf, int socket) {
    // 状态行
    char tmp[1024] = {0};
    int code = static_cast<int>(m_statusCode);
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
    sendBuf->appendString(tmp);
    // 响应头
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        bzero(tmp, sizeof(tmp));
        sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
        sendBuf->appendString(tmp);
    }
    // 空行
    sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(socket);
#endif  // MSG_SEND_AUTO

    DEBUG("回复的数据: %s", m_fileName.data());
    // 回复的数据
    m_sendDataFunc(m_fileName, sendBuf, socket);
}

