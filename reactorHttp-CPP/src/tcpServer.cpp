#include "tcpServer.h"
#include <arpa/inet.h>
#include "tcpConnection.h"
#include "log.h"

TcpServer::TcpServer(unsigned short port, int threadNum): m_port(port), m_threadNum(threadNum) {
    setListen();
    m_mainLoop = new EventLoop;
    m_threadPool = new ThreadPool(m_mainLoop, threadNum);
    
}

TcpServer::~TcpServer() {
}

void TcpServer::setListen() {
    // 1. 创建监听的fd
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);  // TCP
    errif_exit(m_lfd == -1, "listenerInit_socket");
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    errif_exit(ret == -1, "listenerInit_setsockopt");
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (struct sockaddr*) &addr, sizeof(addr));
    errif_exit(ret == -1, "listenerInit_bind");
    // 4. 设置监听
    ret = listen(m_lfd, 128);
    errif_exit(ret == -1, "listenerInit_listen");
}

void TcpServer::run() {
    // 启动线程池
    m_threadPool->run();
    // 添加检测任务
    Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, this);
    m_mainLoop->addTask(channel, ElemType::ADD);
    // 启动反应堆模型
    m_mainLoop->run();
    DEBUG("服务器启动成功!");
}

int TcpServer::acceptConnection(void* arg) {
    TcpServer* server = static_cast<TcpServer*>(arg);
    int cfd = accept(server->m_lfd, NULL, NULL);
    // 从线程池中取出一个子线程的反应堆实例
    auto evLoop = server->m_threadPool->takeWorkerEventLoop();
    DEBUG("mainThread accept, exec threadId: %d", evLoop->getThreadId());
    // 将cfd放到TcpConnection中处理
    // tcpConnectionInit(cfd, evLoop);
    return 0;
}
