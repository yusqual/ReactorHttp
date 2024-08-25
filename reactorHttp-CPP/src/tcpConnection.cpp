#include "tcpConnection.h"
#include "httpRequest.h"
#include "log.h"

int processRead(void* arg) {
    struct TcpConnection* conn = (struct TcpConnection*) arg;
    // 接收数据
    int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
    DEBUG("接收到的http请求数据: %s", conn->readBuf->data + conn->readBuf->readPos);
    if (count > 0) {
        // 接收到了http请求
        // 调用httpRequest
        int socket = conn->channel->fd;
        bool res = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
        if (!res) {
            // 解析失败
            char* errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->writeBuf, errMsg);
        }
#ifdef MSG_SEND_AUTO
        modifyWriteEvent(conn->channel, true);
        eventLoopAddTask(conn->evLoop, conn->channel, MOD);
#endif
    }
    // 断开连接
    // 添加 从检测集合中删掉fd 的任务
#ifndef MSG_SEND_AUTO
    eventLoopAddTask(conn->evLoop, conn->channel, DEL);
    tcpConnectionDestroy(conn);
#endif  // MSG_SEND_AUTO
}

int processWrite(void* arg) {
    DEBUG("开始发送数据...");
    struct TcpConnection* conn = (struct TcpConnection*) arg;
    // 发送数据
    int count = bufferSendData(conn->writeBuf, conn->channel->fd);
    if (count > 0) {
        // 判断数据是否完全发送
        if (bufferReadableSize(conn->writeBuf) == 0) {
            // 不再检测写事件 - 修改channel中保存的事件
            modifyWriteEvent(conn->channel, false);
            // 修改dispatcher检测的集合 - 添加任务
            eventLoopAddTask(conn->evLoop, conn->channel, MOD);
            // 删除节点
            eventLoopAddTask(conn->evLoop, conn->channel, DEL);
            // 释放内存
            tcpConnectionDestroy(conn);
        }
    }
    return 0;
}

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop) {
    struct TcpConnection* conn = (struct TcpConnection*) malloc(sizeof(struct TcpConnection));
    errif_exit(conn == NULL, "tcpConnectionInit");
    conn->evLoop = evLoop;
    conn->readBuf = bufferInit(10240);
    conn->writeBuf = bufferInit(10240);
    // http request response
    conn->request = httpRequestInit();
    conn->response = httpResponseInit();
    sprintf(conn->name, "Connection-%d", fd);
    conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, conn);
    eventLoopAddTask(evLoop, conn->channel, ADD);
    DEBUG("和客户端建立连接, threadName: %s, threadId: %ld, connName: %s", evLoop->threadName, evLoop->threadId, conn->name);
    return conn;
}

void tcpConnectionDestroy(struct TcpConnection* conn) {
    if (conn) {
    DEBUG("连接断开,释放资源: connName: %s, %d", conn->name, conn->channel->fd);
        if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0 &&
            conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0) {
            destroyChannel(conn->evLoop, conn->channel);
            bufferDestroy(conn->readBuf);
            bufferDestroy(conn->writeBuf);
            httpRequestDestroy(conn->request);
            httpResponseDestroy(conn->response);
            free(conn);
        }
    }
}
