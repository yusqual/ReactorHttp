#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include "dispatcher.h"
#include "threadPool.h"
#include "channel.h"
#include <thread>
#include <queue>
#include <mutex>
#include <map>

// 处理节点中channel的方式
enum class ElemType : char { ADD,
                             DEL,
                             MOD };

// 定义任务队列的节点
struct ChannelElement {
    ElemType type;  // 如何处理该节点中的channel
    Channel* channel;
};
class Dispatcher;
class ThreadPool;
class EventLoop {
public:
    EventLoop();
    EventLoop(const std::string threadName, ThreadPool* pool);
    ~EventLoop();
    // 启动反应堆模型
    bool run();
    // 处理激活的文件描述符
    bool eventActive(int fd, int event);
    // 添加任务到任务队列
    bool addTask(struct Channel* channel, ElemType type);
    // 处理任务队列中的任务
    bool processTaskQ();
    // 处理dispatcher中的节点
    bool add(Channel* channel);
    bool del(Channel* channel);
    bool mod(Channel* channel);
    // 释放channel
    bool destroyChannel(struct Channel* channel);
    int readLocalMsg();
    // 获取线程id
    inline std::thread::id getThreadId() { return m_threadId; }
    // 获取socketPair[1]
    inline int getSocketPair() { return m_socketPair[1]; }

private:
    void taskWakeup();

private:
    bool m_isRunning;          // eventloop是否启动
    Dispatcher* m_dispatcher;  // select poll epoll 指向谁就使用谁, 指向子类实例.
    // 任务队列
    std::queue<ChannelElement*> m_taskQ;
    // map
    std::map<int, Channel*> m_channelMap;
    // 线程id, name, mutex
    std::thread::id m_threadId;
    std::string m_threadName;
    std::mutex m_mutex;   // 保护任务队列
    int m_socketPair[2];  // 存储本地通信的fd,通过socketpair初始化,用于将dispatcher从检测函数返回
};

#endif  // _EVENTLOOP_H_