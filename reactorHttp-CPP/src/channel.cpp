#include "channel.h"

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
    : m_fd(fd), m_events(static_cast<int>(events)), m_arg(arg), readCallback(readFunc), writeCallback(writeFunc), destroyCallback(destroyFunc) {}

void Channel::modifyWriteEvent(bool flag) {
    if (flag) m_events |= static_cast<int>(FDEvent::WriteEvent);
    else m_events = m_events & ~(int)FDEvent::WriteEvent;
}

bool Channel::isWriteEventEnable() {
    return m_events & (int)FDEvent::WriteEvent;
}
