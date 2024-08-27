#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "channel.h"
#include "eventLoop.h"
class EventLoop;
class Dispatcher {
public:
    // 初始化 -- 初始化epoll poll 或 select 所需的数据块(dispatcherData)
    Dispatcher(EventLoop* evLoop);
    // 清除数据(关闭fd或者释放内存)
    virtual ~Dispatcher();
    // 添加
    virtual bool add();
    // 删除
    virtual bool remove();
    // 修改
    virtual bool modify();
    // 事件检测, timeout超时时长(ms)
    virtual bool dispatch(int timeout = 2000);
    // 更新channel
    inline void setChannel(Channel* channel) { m_channel = channel; }

protected:
    std::string m_name = std::string();
    Channel* m_channel;
    EventLoop* m_evLoop;
};

#endif  // _DISPATCHER_H_