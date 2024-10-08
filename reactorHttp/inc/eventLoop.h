#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include "dispatcher.h"
#include "channelMap.h"
#include <pthread.h>
#include "threadPool.h"

extern struct Dispatcher epollDispatcher;   // extern关键字用于在其他文件中使用某文件中的全局变量
extern struct Dispatcher pollDispatcher;
extern struct Dispatcher selectDispatcher;
struct ThreadPool;
// 处理节点中channel的方式
enum ElemType{ADD, DEL, MOD};

// 定义任务队列的节点
struct ChannelElement {
    int type;   // 如何处理该节点中的channel
    struct Channel* channel;
    struct ChannelElement* next;
};
struct Dispatcher;
struct EventLoop {
    bool isRunning;    // eventloop是否启动
    struct Dispatcher* dispatcher;  // select poll epoll 指向谁就使用谁
    void* dispatcherData;   // 对应的数据块
    // 任务队列
    struct ChannelElement* head;
    struct ChannelElement* tail;
    // channelMap
    struct ChannelMap* channelmap;
    // 线程id, name, mutex
    pthread_t threadId;
    char threadName[32];
    pthread_mutex_t mutex;  // 保护任务队列
    int socketPair[2];  // 存储本地通信的fd,通过socketpair初始化,用于将dispatcher从检测函数返回
};

// 初始化
struct EventLoop* eventLoopInit();  // 主线程
struct EventLoop* eventLoopInitEx(const char* threadName, struct ThreadPool* pool);// 子线程
// 启动反应堆模型
bool eventLoopRun(struct EventLoop* evLoop);
// 处理激活的文件描述符
bool eventActivate(struct EventLoop* evLoop, int fd, int event);
// 添加任务到任务队列
bool eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);
// 处理任务队列中的任务
bool eventLoopProcessTask(struct EventLoop* evLoop);
// 处理dispatcher中的节点
bool eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
bool eventLoopDel(struct EventLoop* evLoop, struct Channel* channel);
bool eventLoopMod(struct EventLoop* evLoop, struct Channel* channel);
// 释放channel
bool destroyChannel(struct EventLoop* evLoop, struct Channel* channel);


#endif // _EVENTLOOP_H_