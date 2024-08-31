#include "tcpConnection.h"
#include "httpRequest.h"
#include "log.h"

TcpConnection::TcpConnection(int fd, EventLoop* evLoop): m_evLoop(evLoop) {
    m_readBuf = new Buffer(10240);
    m_writeBuf = new Buffer(10240);
    m_request = new HttpRequest;
    m_response = new HttpResponse;
    m_name = "Connection-" + std::to_string(fd);
    m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, destroy, this);
    evLoop->addTask(m_channel, ElemType::ADD);
    DEBUG("和客户端建立连接, threadId: %ld, connName: %s", evLoop->getThreadId(), m_name.data());
}

TcpConnection::~TcpConnection() {
    DEBUG("连接断开,释放资源: connName: %s, %d", m_name.data(), m_channel->getSocket());
    if (m_readBuf && m_readBuf->readableSize() == 0 &&
        m_writeBuf && m_writeBuf->readableSize() == 0) {
        delete m_readBuf;
        delete m_writeBuf;
        delete m_request;
        delete m_response;
        m_evLoop->destroyChannel(m_channel);
    }
}

int TcpConnection::processRead(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    // 接收数据
    int socket = conn->m_channel->getSocket();
    int count = conn->m_readBuf->socketRead(socket);
    DEBUG("接收到的http请求数据: %s", conn->m_readBuf->data());
    if (count > 0) {
        // 接收到了http请求
        // 调用httpRequest
        bool res = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
        if (!res) {
            // 解析失败
            const char* errMsg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuf->appendString(errMsg);
        }
#ifdef MSG_SEND_AUTO
        conn->m_channel->modifyWriteEvent(true);
        conn->m_evLoop->addTask(conn->m_channel, ElemType::MOD);
#endif
    }
    // 断开连接
    // 添加 从检测集合中删掉fd 的任务
#ifndef MSG_SEND_AUTO
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DEL);
#endif  // MSG_SEND_AUTO
    return 0;
}

int TcpConnection::processWrite(void* arg) {
    DEBUG("开始发送数据...");
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    // 发送数据
    int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
    if (count > 0) {
        // 判断数据是否完全发送
        if (conn->m_writeBuf->readableSize() == 0) {
            conn->m_channel->modifyWriteEvent(false);
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MOD);
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DEL);
        }
    }
    return 0;
}

int TcpConnection::destroy(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    if (conn != nullptr) delete conn;
    return 0;
}