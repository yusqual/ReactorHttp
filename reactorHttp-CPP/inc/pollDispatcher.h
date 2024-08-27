#ifndef _POLLDISPATCHER_H_
#define _POLLDISPATCHER_H_

#include "channel.h"
#include "eventLoop.h"
#include "dispatcher.h"
#include <poll.h>

class PollDispatcher: public Dispatcher {
public:
    // 初始化 -- 初始化epoll poll 或 select 所需的数据块(dispatcherData)
    PollDispatcher(EventLoop* evLoop);
    // 清除数据(关闭fd或者释放内存)
    ~PollDispatcher();
    // 添加
    bool add();
    // 删除
    bool remove();
    // 修改
    bool modify();
    // 事件检测, timeout超时时长(ms)
    bool dispatch(int timeout = 2000);

private:
    int m_maxfd;
    struct pollfd* m_fds;
    const int m_maxNode = 1024;
};

#endif  // _POLLDISPATCHER_H_