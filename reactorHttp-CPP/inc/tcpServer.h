#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "eventLoop.h"
#include "threadPool.h"

class TcpServer {
public:
    TcpServer(unsigned short port, int threadNum);
    ~TcpServer();
    void setListen();
    void run();
    static int acceptConnection(void* arg);

private:
    EventLoop* m_mainLoop;
    ThreadPool* m_threadPool;
    int m_threadNum;
    int m_lfd;
    unsigned short m_port;
};

#endif  // _TCPSERVER_H_