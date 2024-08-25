#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "eventLoop.h"
#include "threadPool.h"

struct Listener {
    int lfd;
    unsigned short port;
};

struct TcpServer {
    struct EventLoop* mainLoop;
    struct ThreadPool* threadPool;
    struct Listener* listener;
    int threadNum;
};

// init tcpserver
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
// init listener
struct Listener* listenerInit(unsigned short port);
// run
void tcpServerRun(struct TcpServer* server);

#endif // _TCPSERVER_H_