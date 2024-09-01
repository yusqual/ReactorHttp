#include "dispatcher.h"
#include <sys/select.h>
#include "selectDispatcher.h"

SelectDispatcher::SelectDispatcher(EventLoop* evLoop): Dispatcher(evLoop) {
    FD_ZERO(&m_readfds);
    FD_ZERO(&m_writefds);
    m_name = "Select";
}

SelectDispatcher::~SelectDispatcher() {
}

bool SelectDispatcher::add() {
    if (m_channel->getSocket() >= m_maxSize) return false;
    addFdSet();
    return true;
}

bool SelectDispatcher::remove() {
    if (m_channel->getSocket() >= m_maxSize) return false;
    clrFdSet();
    // 通过channel释放对应的TcpConnection资源
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
    return true;
}

bool SelectDispatcher::modify() {
    if (m_channel->getSocket() >= m_maxSize) return false;
    if (m_channel->getEvent() & (int) FDEvent::ReadEvent) {
        FD_SET(m_channel->getSocket(), &m_readfds);
        FD_CLR(m_channel->getSocket(), &m_writefds);
    }
    if (m_channel->getEvent() & (int) FDEvent::WriteEvent) {
        FD_SET(m_channel->getSocket(), &m_writefds);
        FD_CLR(m_channel->getSocket(), &m_readfds);
    }
    return true;
}

bool SelectDispatcher::dispatch(int timeout) {
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = m_readfds;
    fd_set wrtmp = m_writefds;
    int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);
    errif_exit(count == -1, "select");
    for (int i = 0; i < m_maxSize; ++i) {
        if (FD_ISSET(i, &rdtmp)) {
            m_evLoop->eventActive(i, (int) FDEvent::ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp)) {
            m_evLoop->eventActive(i, (int) FDEvent::WriteEvent);
        }
    }
    return true;
}

void SelectDispatcher::addFdSet() {
    if (m_channel->getEvent() & (int) FDEvent::ReadEvent) FD_SET(m_channel->getSocket(), &m_readfds);
    if (m_channel->getEvent() & (int) FDEvent::WriteEvent) FD_SET(m_channel->getSocket(), &m_writefds);
}

void SelectDispatcher::clrFdSet() {
    if (!(m_channel->getEvent() & (int) FDEvent::ReadEvent)) FD_CLR(m_channel->getSocket(), &m_readfds);
    if (!(m_channel->getEvent() & (int) FDEvent::WriteEvent)) FD_CLR(m_channel->getSocket(), &m_writefds);
}
