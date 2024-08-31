#include "eventLoop.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "log.h"
#include "selectDispatcher.h"
#include "pollDispatcher.h"
#include "epollDispatcher.h"

EventLoop::EventLoop(): EventLoop(std::string(), nullptr) {
}

EventLoop::EventLoop(const std::string threadName, ThreadPool* pool) {
    m_isRunning = false;
    m_threadId = std::this_thread::get_id();
    m_threadName = threadName == std::string() ? "MainThread" : threadName;
    m_dispatcher = new EpollDispatcher(this);  // 使用epoll
    m_channelMap.clear();
    if (threadName == std::string()) {
        int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
        errif_exit(ret == -1, "eventLoopInitEx_socketpair");
        // 指定evLoop->socketPair[0]发送数据,[1]接收数据
        // 添加[1]到dipatcher检测列表中, 设置读事件原因: epoll默认采用水平触发, 若不读取读缓冲区数据则会一直触发

        // std::bind - 绑定
        auto obj = std::bind(&EventLoop::readLocalMsg, this);
        Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, obj, nullptr, nullptr, this);
        addTask(channel, ElemType::ADD);  // 添加channel到任务队列
    } else {                              // 子线程把主线程的socketPair[1]放到自己的evloop里面
        auto obj = std::bind(&EventLoop::readLocalMsg, this);
        Channel* channel = new Channel(pool->getMainLoop()->getSocketPair(), FDEvent::ReadEvent, obj, nullptr, nullptr, this);
        addTask(channel, ElemType::ADD);  // 添加channel到任务队列
    }
}

EventLoop::~EventLoop() {
}

bool EventLoop::run() {
    m_isRunning = true;
    // 检测线程id是否正常
    if (m_threadId != std::this_thread::get_id()) return false;
    // 取出事件分发和检测模型
    while (m_isRunning) {
        m_dispatcher->dispatch();  // 超时时长2s
        processTaskQ();
    }
    return true;
}

bool EventLoop::eventActive(int fd, int event) {
    if (fd < 0) return false;
    // 取出channel
    Channel* channel = m_channelMap[fd];
    assert(channel->getSocket() == fd);
    if (event & (int) FDEvent::ReadEvent && channel->readCallback) channel->readCallback(const_cast<void*>(channel->getArg()));
    if (event & (int) FDEvent::WriteEvent && channel->writeCallback) channel->writeCallback(const_cast<void*>(channel->getArg()));
    return true;
}

bool EventLoop::addTask(Channel* channel, ElemType type) {
    // 加锁, 保护共享资源
    m_mutex.lock();
    // 创建新节点
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    m_taskQ.push(node);
    m_mutex.unlock();
    // 处理节点
    /*
     * 细节:
     *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
     *       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
     *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
     *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
     */
    if (m_threadId == std::this_thread::get_id()) {
        // 当前是子(主)线程, 处理任务队列中的任务
        processTaskQ();
    } else {
        // 当前是主线程 -- 告诉子线程处理任务队列中的任务
        // 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
        // 通过在dispatcher检测的fd中添加一个单独的fd,来控制dispatcher检测函数能够直接返回
        taskWakeup();
        DEBUG("发出socketPair[0].");
    }
    return true;
}

bool EventLoop::processTaskQ() {
    while (!m_taskQ.empty()) {
        m_mutex.lock();
        ChannelElement* node = m_taskQ.front();
        m_taskQ.pop();
        m_mutex.unlock();
        Channel* channel = node->channel;
        if (node->type == ElemType::ADD) {
            add(channel);
        } else if (node->type == ElemType::DEL) {
            del(channel);
        } else if (node->type == ElemType::MOD) {
            mod(channel);
        }
        delete node;
    }
    return true;
}
bool EventLoop::add(Channel* channel) {
    int fd = channel->getSocket();
    // 找到fd对应的数组元素位置并存储
    if (m_channelMap.find(fd) == m_channelMap.end()) {
        m_channelMap.insert(std::make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        return m_dispatcher->add();
    }
    return false;
}
bool EventLoop::del(Channel* channel) {
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end()) return false;
    // 找到fd对应的数组元素位置并存储
    m_dispatcher->setChannel(channel);
    return m_dispatcher->remove();
}
bool EventLoop::mod(Channel* channel) {
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end()) return false;
    m_dispatcher->setChannel(channel);
    return m_dispatcher->modify();
}
bool EventLoop::destroyChannel(Channel* channel) {
    auto it = m_channelMap.find(channel->getSocket());
    if (it != m_channelMap.end()) {
        m_channelMap.erase(it);
        close(channel->getSocket());
        delete channel;
    }
    return true;
}

int EventLoop::readLocalMsg() {
    char buf[256];
    int res = read(m_socketPair[1], buf, sizeof(buf));
    return res;
}

void EventLoop::taskWakeup() {
    const char* msg = "Get up!";
    write(m_socketPair[0], msg, strlen(msg));
}
