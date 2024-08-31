#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_

#include "eventLoop.h"
#include "buffer.h"
#include "httpRequest.h"

// #define MSG_SEND_AUTO

class TcpConnection {
public:
    TcpConnection(int fd, struct EventLoop* evLoop);
    ~TcpConnection();

    static int processRead(void* arg);
    static int processWrite(void* arg);
    static int destroy(void* arg);

private:
    EventLoop* m_evLoop;
    Channel* m_channel;
    Buffer* m_readBuf;
    Buffer* m_writeBuf;
    std::string m_name;

    HttpRequest* m_request;
    HttpResponse* m_response;
};

#endif // _TCPCONNECTION_H_