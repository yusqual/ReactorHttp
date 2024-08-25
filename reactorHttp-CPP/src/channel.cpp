#include "channel.h"

struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg) {
    struct Channel* channel = (struct Channel*) malloc(sizeof(struct Channel));
    errif_exit(channel == NULL, "channel malloc");
    channel->fd = fd;
    channel->events = events;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->arg = arg;
    return channel;
}

void modifyWriteEvent(struct Channel* channel, bool flag) {
    if (flag)
        channel->events |= WriteEvent;
    else
        channel->events = channel->events & ~WriteEvent;    // ~WriteEvent: 1...111011
}

bool isWriteEventEnable(struct Channel* channel) {
    return channel->events & WriteEvent;
}
