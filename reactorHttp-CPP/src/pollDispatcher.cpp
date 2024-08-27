#include "dispatcher.h"
#include <sys/poll.h>
#include "pollDispatcher.h"


PollDispatcher::PollDispatcher(EventLoop* evLoop): Dispatcher(evLoop), m_maxfd(0) {
    m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; ++i) {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher() {
    delete[] m_fds;
}

bool PollDispatcher::add() {
    int events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent) events |= POLLIN;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent) events |= POLLOUT;
    for (int i = 0; i < m_maxNode; ++i) {
        if (m_fds[i].fd == -1) {
            m_fds[i].fd = m_channel->getSocket();
            m_fds[i].events = events;
            m_maxfd = m_maxfd < i ? i : m_maxfd;
            return true;
        }
    }
    return false;
}

bool PollDispatcher::remove() {
    for (int i = 0; i < m_maxNode; ++i) {
        if (m_fds[i].fd == m_channel->getSocket()) {
            m_fds[i].fd = -1;
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            return true;
        }
    }
    // 通过channel释放对应的TcpConnection资源
    // m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
    return false;
}

bool PollDispatcher::modify() {
    int events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent) events |= POLLIN;
    if (m_channel->getEvent() & (int)FDEvent::WriteEvent) events |= POLLOUT;
    for (int i = 0; i < m_maxNode; ++i) {
        if (m_fds[i].fd == m_channel->getSocket()) {
            m_fds[i].events = events;
            return true;
        }
    }
    return false;
}

bool PollDispatcher::dispatch(int timeout) {
    int count = poll(m_fds, m_maxfd + 1, timeout);
    errif_exit(count == -1, "poll");
    for (int i = 0, done = 0; i <= m_maxfd && done < count; ++i) {
        if (m_fds[i].fd == -1) continue;
        if (m_fds[i].revents & POLLIN) {
            // eventActivate(m_evLoop, data->fds[i].fd, (int)FDEvent::ReadEvent);
        }
        if (m_fds[i].revents & POLLOUT) {
            // eventActivate(m_evLoop, data->fds[i].fd, (int)FDEvent::WriteEvent);
        }
        ++done;
    }
    return true;
}
