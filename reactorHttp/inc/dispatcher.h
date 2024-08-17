#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "channel.h"
#include "eventLoop.h"

struct Dispatcher {
    // 初始化 -- 初始化epoll poll 或 select 所需的数据块(dispatcherData)
    void* (*init)();
    // 添加
    int (*add)(struct Channel* channel, struct EventLoop* eventLoop);
    // 删除
    int (*remove)(struct Channel* channel, struct EventLoop* eventLoop);
    // 修改
    int (*modify)(struct Channel* channel, struct EventLoop* eventLoop);
    // 事件检测
    int (*dispatch)(struct EventLoop* eventLoop, int timeout);
    // 清除数据(关闭fd或者释放内存)
    int (*clear)(struct EventLoop* eventLoop);
};

#endif  // _DISPATCHER_H_