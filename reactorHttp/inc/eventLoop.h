#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include "dispatcher.h"

struct EventLoop {
    struct Dispatcher* dispatcher;
    void* dispatcherData;
};

#endif // _EVENTLOOP_H_