#include "dispatcher.h"

Dispatcher::Dispatcher(EventLoop* evLoop): m_evLoop(evLoop) {
}

Dispatcher::~Dispatcher() {
}

bool Dispatcher::add() {
    return false;
}

bool Dispatcher::remove() {
    return false;
}

bool Dispatcher::modify() {
    return false;
}

bool Dispatcher::dispatch(int timeout) {
    return false;
}
