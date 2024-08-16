#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cassert>
#include <sys/sendfile.h>
#include <dirent.h>
#include <ctype.h>

struct sockInfo {
    int fd;
    int epfd;
};

int initListenFd(unsigned short port) {
    // 1. 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    errif_exit(lfd == -1, "socket");
    // 2. 设置端口复用
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &lfd, sizeof(lfd));
    errif_exit(ret == -1, "setsockopt");
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr*) &addr, sizeof(addr));
    errif_exit(ret == -1, "bind");
    // 4. 设置监听
    ret = listen(lfd, 128);
    errif_exit(ret == -1, "listen");
    // 返回fd
    return lfd;
}

int epollRun(int lfd, ThreadPool* pool) {
    // 1. 创建epoll实例
    int epfd = epoll_create(1);
    errif_exit(epfd == -1, "epoll_create");
    // 2. lfd 上树
    struct epoll_event ev;  // 添加到epoll树的结构体
    ev.data.fd = lfd;
    ev.events = EPOLLIN | EPOLLET;  // 检测读事件, 使用线程池后需改为边缘模式, 具体原因不明
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    errif_exit(ret == -1, "epoll_ctl add");
    // 3. 监听
    struct epoll_event evs[1024];
    int size_evs = sizeof(evs) / sizeof(struct epoll_event);
    while (true) {
        int num = epoll_wait(epfd, evs, size_evs, -1);
        for (int i = 0; i < num; ++i) {
            int curfd = evs[i].data.fd;
            struct sockInfo* info = (struct sockInfo*)malloc(sizeof(struct sockInfo));
            info->fd = curfd;
            info->epfd = epfd;
            if (curfd == lfd) {  // 监听的fd
                // acceptClient(epfd, curfd);
                threadPoolAdd(pool, acceptClient, info);
            } else {
                // 主要是接收对端的数据
                // recvHttpRequest(epfd, curfd);
                threadPoolAdd(pool, recvHttpRequest, info);
            }
        }
    }

    return 0;
}

// int acceptClient(int epfd, int lfd) {
void acceptClient(void* arg) {
    struct sockInfo* info = (struct sockInfo*)arg;
    // 1. 建立连接
    int cfd = accept(info->fd, NULL, NULL);
    if (cfd == -1) {
        perror("accept client");
        free(info);
        return;
    }
    // 2. 设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    // 3. cfd添加到epoll
    struct epoll_event ev;  // 添加到epoll树的结构体
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET;  // 边缘模式检测读事件
    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1) {
        perror("epoll_ctl add client");
        free(info);
        return;
    }
    printf("accept threadId: %ld\n", pthread_self());
    return;
}

// int recvHttpRequest(int epfd, int cfd) {
void recvHttpRequest(void* arg) {
    struct sockInfo* info = (struct sockInfo*)arg;
    char buf[4096] = {0};
    char tmp[1024] = {0};
    int len = 0, total = 0;
    while ((len = recv(info->fd, tmp, sizeof(tmp), 0)) > 0) {
        if (total + len < sizeof(buf)) {
            memcpy(buf + total, tmp, len);
        }
        total += len;
        bzero(tmp, sizeof(tmp));
    }

    // 判断数据是否接收完毕
    if (len == -1 && errno == EAGAIN) {
        // 解析请求行(项目仅处理GET请求)
        char* pt = strstr(buf, "\r\n");
        int reqLen = pt - buf;
        buf[reqLen] = '\0';
        parseRequestLine(buf, info->fd);

    } else if (len == 0) {
        // 客户端断开连接
        epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
        close(info->fd);
    } else {
        perror("recvHttpRequest recv");
        free(info);
        return;
    }
    close(info->fd);
    printf("recvMsg threadId: %ld\n", pthread_self());
    return;
}

int parseRequestLine(const char* line, int cfd) {
    // 解析请求行       GET /xxx/xxx.x http/1.1
    char method[12];  // GET/POST, 不会太大
    char path[1024];  // 请求资源的路径
    path[0] = '.';    // 首部加.转为相对路径
    sscanf(line, "%[^ ] %[^ ]", method, path + 1);
    if (strcasecmp(method, "get") != 0) {  // strcasecmp 不区分大小写
        return -1;
    }
    decodeMsg(path, path);

    printf("method: %s, path: %s\n", method, path);

    // 获取文件属性, 判断是文件还是目录
    struct stat st;
    int ret = stat(path, &st);
    if (ret == -1) {
        // 文件不存在 -> 回复404
        sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        sendFile("404.html", cfd);
        return 0;
    }
    // 判断是否是目录
    if (S_ISDIR(st.st_mode)) {
        // 目录, 返回目录中的内容
        sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        sendDir(path, cfd);
    } else {
        // 文件, 返回文件内容
        sendHeadMsg(cfd, 200, "OK", getFileType(path), st.st_size);
        sendFile(path, cfd);
    }
    return 0;
}

int sendFile(const char* file, int cfd) {
    // 1. 打开文件
    int fd = open(file, O_RDONLY);
    assert(fd > 0);
#if 0
    char buf[1024];
    while (true) {
        bzero(buf, sizeof(buf));
        int len = read(fd, buf, sizeof(buf));
        if (len > 0) {
            send(cfd, buf, len, 0);
            usleep(1);  // 减缓接收端压力
        } else if (len < 0) {
            perror("sendFile read");
            return -1;
        } else break;
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
    return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length) {
    // 状态行
    char buf[4096] = {0};
    sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
    // 响应头
    sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
    sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

    send(cfd, buf, strlen(buf), 0);
    return 0;
}

/*
<html>
    <head>
        <title>test</title>
    </head>
    <body>
        <table>
            <tr>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td></td>
                <td></td>
            </tr>
        </table>
    </body>
</html>
*/

int sendDir(const char* dir, int cfd) {
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir);
    struct dirent** namelist;
    int num = scandir(dir, &namelist, NULL, alphasort);
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
        send(cfd, buf, strlen(buf), 0);
        bzero(buf, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
    free(namelist);
    return 0;
}

// 根据文件后缀返回http响应头中的content-type
const char* getFileType(const char* name) {
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
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
