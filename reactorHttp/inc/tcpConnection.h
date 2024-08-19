#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_

#include "eventLoop.h"
#include "buffer.h"

struct TcpConnection {
    struct EventLoop* evLoop;
    struct Channel* channel;
    struct Buffer* readBuf;
    struct Buffer* writeBuf;
    char name[32];
};

// init
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);

#endif // _TCPCONNECTION_H_