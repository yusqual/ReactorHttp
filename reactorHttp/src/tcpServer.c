#include "tcpServer.h"
#include <arpa/inet.h>
#include "tcpConnection.h"
#include "log.h"


struct TcpServer* tcpServerInit(unsigned short port, int threadNum) {
    struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    errif_exit(tcp == NULL, "tcpServerInit", true);
    tcp->listener = listenerInit(port);
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
    return tcp;
}

struct Listener* listenerInit(unsigned short port) {
    struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
    errif_exit(listener == NULL, "listenerInit_1", true);
    // 1. 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    errif_exit(lfd == -1, "listenerInit_socket", true);
    // 2. 设置端口复用
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &lfd, sizeof(lfd));
    errif_exit(ret == -1, "listenerInit_setsockopt", true);
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr*) &addr, sizeof(addr));
    errif_exit(ret == -1, "listenerInit_bind", true);
    // 4. 设置监听
    ret = listen(lfd, 128);
    errif_exit(ret == -1, "listenerInit_listen", true);
    // 返回fd
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}

int acceptConnection(void* arg) {
    struct TcpServer* server = (struct TcpServer*)arg;
    int cfd = accept(server->listener->lfd, NULL, NULL);
    // 从线程池中取出一个子线程的反应堆实例
    struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
    // 将cfd放到TcpConnection中处理
    tcpConnectionInit(cfd, evLoop);
    return 0;
}

void tcpServerRun(struct TcpServer* server) {
    // 启动线程池
    threadPoolRun(server->threadPool);
    // 添加检测任务
    struct Channel* channel = channelInit(server->listener->lfd, ReadEvent, acceptConnection, NULL, server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    // 启动反应堆模型
    eventLoopRun(server->mainLoop);
    DEBUG("服务器启动成功!");
}
