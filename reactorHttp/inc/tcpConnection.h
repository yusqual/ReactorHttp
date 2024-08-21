#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_

#include "eventLoop.h"
#include "buffer.h"
#include "httpRequest.h"

// #define MSG_SEND_AUTO

struct TcpConnection {
    struct EventLoop* evLoop;
    struct Channel* channel;
    struct Buffer* readBuf;
    struct Buffer* writeBuf;
    char name[32];

    struct HttpRequest* request;
    struct HttpResponse* response;
};

// init
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);
void tcpConnectionDestroy(struct TcpConnection* conn);


#endif // _TCPCONNECTION_H_