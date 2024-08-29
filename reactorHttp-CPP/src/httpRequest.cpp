#define _GNU_SOURCE
#include "httpRequest.h"
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <assert.h>
#include <ctype.h>
#include "log.h"

HttpRequest::HttpRequest() {
    reset();
}

HttpRequest::~HttpRequest() {
}

void HttpRequest::reset() {
    m_curState = ProcessStatus::ParseReqLine;
    m_method = m_url = m_version = std::string();
    m_reqHeaders.clear();
}

void HttpRequest::addHeader(const std::string key, const std::string value) {
    if (key.empty() || value.empty()) return;
    m_reqHeaders.insert(std::make_pair(key, value));
}

std::string HttpRequest::getHeader(const std::string key) {
    auto item = m_reqHeaders.find(key);
    if (item != m_reqHeaders.end()) return item->second;
    return std::string();
}

bool HttpRequest::parseRequestLine(Buffer* readBuf) {
    // 读出请求行, 保存字符串结束地址
    char* end = readBuf->findCRLF();
    // 保存字符串起始地址
    char* start = readBuf->data();
    // 请求行总长度
    int lineLen = end - start;
    if (lineLen) {
        // Get /xxx/xx.xx http/x.x
        // 请求方式
        auto methodFunc = std::bind(&HttpRequest::setMethod, this, std::placeholders::_1);
        auto urlFunc = std::bind(&HttpRequest::setUrl, this, std::placeholders::_1);
        auto verFunc = std::bind(&HttpRequest::setVersion, this, std::placeholders::_1);
        start = splitRequestLine(start, end, " ", methodFunc);
        start = splitRequestLine(start, end, " ", urlFunc);
        splitRequestLine(start, end, NULL, verFunc);
        // 为解析请求头做准备
        readBuf->readPosIncrease(lineLen + 2);  // 2: "\r\n"

        // 修改状态
        setStatus(ProcessStatus::ParseReqHeaders);
        return true;
    }
    return false;
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf) {
    char* end = readBuf->findCRLF();
    if (end) {
        char* start = readBuf->data();
        int lineLen = end - start;
        // 基于:搜索字符串
        char* middle = static_cast<char*>(memmem(start, lineLen, ": ", 2));
        if (middle != nullptr) {
            int keyLen = middle - start;      // key长度
            int valueLen = end - middle - 2;  // value长度
            if (keyLen > 0 && valueLen > 0) {
                std::string key(start, keyLen);
                std::string value(middle + 2, valueLen);
                addHeader(key, value);
            }

            // 移动读数据位置
            readBuf->readPosIncrease(lineLen + 2);
        } else {
            // 请求头解析完毕
            readBuf->readPosIncrease(2);
            // 修改解析状态
            // 忽略POST请求,按照GET设置
            m_curState = ProcessStatus::ParseReqDone;
        }
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket) {
    bool result = true;
    while (m_curState != ProcessStatus::ParseReqDone) {
        switch (m_curState) {
            case ProcessStatus::ParseReqLine:
                result = parseRequestLine(readBuf);
                break;
            case ProcessStatus::ParseReqHeaders:
                result = parseRequestHeader(readBuf);
                break;
            case ProcessStatus::ParseReqBody:
                printf("POST REQUEST.\n");
                break;
            default:
                break;
        }
    }
    if (result && m_curState == ProcessStatus::ParseReqDone) {
        // 1. 根据解析出的数据,对客户端的请求做出处理
        int res = processHttpRequest(response);
        if (res == false) {
            result = false;
        } else {
            DEBUG("解析数据完成.");
            // 2. 组织响应数据并发送给客户端
            // httpResponsePrepareMsg(response, sendBuf, socket);
        }
    }
    m_curState = ProcessStatus::ParseReqLine;  // 状态还原, 保证能处理后面的http请求
    return result;
}

bool HttpRequest::processHttpRequest(HttpResponse* response) {
    if (strcasecmp(m_method.data(), "get") != 0) {  // strcasecmp 不区分大小写
        return false;
    }
    m_url = decodeMsg(m_url);
    char* path = (char*) malloc(m_url.size() + 2);
    // 格式化访问的资源路径, 在最前面加上'.'来访问本地目录
    path[0] = '.';
    path[m_url.size() + 1] = '\0';
    memcpy(path + 1, m_url.data(), m_url.size());

    printf("method: %s, path: %s, request.url: %s\n", m_method, path, m_url);

    // 获取文件属性, 判断是文件还是目录
    struct stat st;
    int ret = stat(path, &st);  // TODO
    // if (ret == -1) {
    //     // 文件不存在 -> 回复404
    //     // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
    //     // sendFile("404.html", cfd);
    //     strcpy(response->fileName, "404.html");
    //     response->statusCode = NotFound;
    //     strcpy(response->statusMsg, "Not Found");
    //     // 响应头
    //     httpResponseAddHeader(response, "Content-type", getFileType(".html"));
    //     response->sendDataFunc = sendFile;
    //     return true;
    // }
    // strcpy(response->fileName, request->url);
    // response->statusCode = OK;
    // strcpy(response->statusMsg, "OK");
    // // 判断是否是目录
    // if (S_ISDIR(st.st_mode)) {
    //     // 目录, 返回目录中的内容
    //     // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
    //     // sendDir(path, cfd);
    //     // 响应头
    //     httpResponseAddHeader(response, "Content-type", getFileType(".html"));
    //     httpResponseAddHeader(response, "Content-length", "-1");
    //     response->sendDataFunc = sendDir;
    // } else {
    //     // 文件, 返回文件内容
    //     // sendHeadMsg(cfd, 200, "OK", getFileType(path), st.st_size);
    //     // sendFile(path, cfd);
    //     // 响应头
    //     char tmp[32] = {0};
    //     sprintf(tmp, "%ld", st.st_size);
    //     httpResponseAddHeader(response, "Content-type", getFileType(request->url));
    //     httpResponseAddHeader(response, "Content-length", tmp);
    //     response->sendDataFunc = sendFile;
    // }
    return true;
}

void HttpRequest::sendFile(const std::string file, Buffer* sendBuf, int cfd) {
    // 1. 打开文件
    int fd = open(file.data(), O_RDONLY);
    assert(fd > 0);
#if 1
    char buf[1024];
    while (true) {
        bzero(buf, sizeof(buf));
        int len = read(fd, buf, sizeof(buf));
        if (len > 0) {
            // send(cfd, buf, len, 0);
            sendBuf->appendString(buf, len);
#ifndef MSG_SEND_AUTO
            sendBuf->sendData(cfd);
#endif  // MSG_SEND_AUTO
        } else if (len < 0) {
            close(fd);
            perror("sendFile read");
            return;
        } else
            break;
    }
#else
    off_t offset = 0;
    int size_file = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size_file) {                         // sendfile中有块缓存,若文件过大,需不断调用该函数.第三个参数会自动改为本次发送的偏移量
        sendfile(cfd, fd, &offset, size_file - offset);  // 使用系统提供函数来发送文件
    }
#endif
    close(fd);
    return;
}

void HttpRequest::sendDir(const std::string dir, Buffer* sendBuf, int cfd) {
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir);
    struct dirent** namelist;
    int num = scandir(dir.data(), &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i) {
        // 取出文件名
        char* name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dir, name);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode)) {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        } else {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        // send(cfd, buf, strlen(buf), 0);
        sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
        sendBuf->sendData(cfd);
#endif  // MSG_SEND_AUTO
        bzero(buf, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(cfd);
#endif  // MSG_SEND_AUTO
    free(namelist);
    return;
}

char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, std::function<void(std::string)> callback) {
    char* space = const_cast<char*>(end);
    if (sub) {
        space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
        errif_exit(space == nullptr, "splitRequestLine_1");
    }
    int length = space - start;  // 请求方式长度 Get Post...
    callback(std::string(start, length));
    return space + 1;
}

// 将字符转换为整形数
int HttpRequest::hexToDec(char c) {
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
std::string HttpRequest::decodeMsg(const std::string msg) {
    const char* from = msg.data();
    std::string str = std::string();
    for (; *from != '\0'; ++from) {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        } else {
            // 字符拷贝, 赋值
            str.append(1, *from);
        }
    }
    str.append(1, '\0');
    return str;
}

// 根据文件后缀返回http响应头中的content-type
const std::string HttpRequest::getFileType(const std::string name) {
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name.data(), '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";  // 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mp3";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";
    if (strcmp(dot, ".pdf") == 0)
        return "application/pdf";

    return "text/plain; charset=utf-8";
}
