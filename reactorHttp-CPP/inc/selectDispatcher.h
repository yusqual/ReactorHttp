#ifndef _SELECTDISPATCHER_H_
#define _SELECTDISPATCHER_H_

#include "channel.h"
#include "eventLoop.h"
#include "dispatcher.h"
#include <sys/select.h>

class SelectDispatcher: public Dispatcher {
public:
    // 初始化 -- 初始化epoll poll 或 select 所需的数据块(dispatcherData)
    SelectDispatcher(EventLoop* evLoop);
    // 清除数据(关闭fd或者释放内存)
    ~SelectDispatcher();
    // 添加
    bool add() override;
    // 删除
    bool remove() override;
    // 修改
    bool modify() override;
    // 事件检测, timeout超时时长(ms)
    bool dispatch(int timeout = 2000) override;

private:
    void addFdSet();
    void clrFdSet();

private:
    fd_set m_readfds;
    fd_set m_writefds;
    const int m_maxSize = 1024;
};

#endif // _SELECTDISPATCHER_H_