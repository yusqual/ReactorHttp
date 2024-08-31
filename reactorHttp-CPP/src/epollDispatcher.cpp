#include "dispatcher.h"
#include <sys/epoll.h>
#include "log.h"
#include "epollDispatcher.h"

EpollDispatcher::EpollDispatcher(EventLoop* evLoop): Dispatcher(evLoop) {
    m_epfd = epoll_create(10);
    errif_exit(m_epfd == -1, "epollInit_epoll_create");
    m_events = new struct epoll_event[m_maxNode];
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher() {
    close(m_epfd);
    delete[] m_events;
}

bool EpollDispatcher::add() {
    bool ret = epollCtl(EPOLL_CTL_ADD);
    errif_exit(ret == false, "epoll_ctl_add", false);
    return ret;
}

bool EpollDispatcher::remove() {
    bool ret = epollCtl(EPOLL_CTL_DEL);
    errif_exit(ret == false, "epoll_ctl_del", false);
    // 通过channel释放对应的TcpConnection资源
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
    return ret;
}

bool EpollDispatcher::modify() {
    bool ret = epollCtl(EPOLL_CTL_MOD);
    errif_exit(ret == false, "epoll_ctl_mod", false);
    return ret;
}

bool EpollDispatcher::dispatch(int timeout) {
    int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout);
    for (int i = 0; i < count; ++i) {
        int events = m_events[i].events;
        int fd = m_events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP) {
            // 对方断开了连接
            // remove(m_evLoop->channelmap->list[fd], eventLoop);
            DEBUG("epollDispatch: fd %d 出现异常, 将其移除...", fd);
            continue;
        }
        if (events & EPOLLIN) {  // 读事件
            m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
        }
        if (events & EPOLLOUT) {  // 写事件
            m_evLoop->eventActive(fd, (int)FDEvent::WriteEvent);
        }
    }
    return true;
}

bool EpollDispatcher::epollCtl(int op) {
    struct epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    // ev.events = channel->events; // 两者events不同,不可直接赋值.channel中的events是我们自己定义的数值
    ev.events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent) ev.events |= EPOLLIN;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent) ev.events |= EPOLLOUT;
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret == 0;
}
