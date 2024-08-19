#include "tcpConnection.h"

int processRead(void* arg) {
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    // 接收数据
    int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
    if (count > 0) {
        // 接收到了http请求

    } else {
        // 断开连接
        
    }
}

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop) {
    struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
    conn->evLoop = evLoop;
    conn->readBuf = bufferInit(10240);
    conn->writeBuf = bufferInit(10240);
    sprintf(conn->name, "Connection-%d", fd);
    conn->channel = channelInit(fd, ReadEvent, processRead, NULL, conn);
    eventLoopAddTask(evLoop, conn->channel, ADD);
    return conn;
}