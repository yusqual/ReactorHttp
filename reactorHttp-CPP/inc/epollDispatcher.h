#ifndef _EPOLLDISPATCHER_H_
#define _EPOLLDISPATCHER_H_

#include "channel.h"
#include "eventLoop.h"
#include "dispatcher.h"
#include <sys/epoll.h>

class EpollDispatcher: public Dispatcher {
public:
    // 初始化 -- 初始化epoll poll 或 select 所需的数据块(dispatcherData)
    EpollDispatcher(EventLoop* evLoop);
    // 清除数据(关闭fd或者释放内存)
    ~EpollDispatcher();
    // 添加
    bool add() override;
    // 删除
    bool remove() override;
    // 修改
    bool modify() override;
    // 事件检测, timeout超时时长(ms)
    bool dispatch(int timeout = 2000) override;

private:
    bool epollCtl(int op);

private:
    int m_epfd;
    struct epoll_event* m_events;
    const int m_maxNode = 520;
};

#endif  // _EPOLLDISPATCHER_H_