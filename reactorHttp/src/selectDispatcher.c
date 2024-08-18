#include "dispatcher.h"
#include <sys/select.h>

static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* eventLoop);
static int selectRemove(struct Channel* channel, struct EventLoop* eventLoop);
static int selectModify(struct Channel* channel, struct EventLoop* eventLoop);
static int selectDispatch(struct EventLoop* eventLoop, int timeout);
static int selectClear(struct EventLoop* eventLoop);

static void addFdSet(struct Channel* channel, struct EventLoop* eventLoop);
static void clrFdSet(struct Channel* channel, struct EventLoop* eventLoop);

const int max_fds_size = 1024;

// select需要的数据块
struct SelectData {
    fd_set readfds;
    fd_set writefds;
};
// select对应的dispatcher
struct Dispatcher selectDispatcher = {
    selectInit,
    selectAdd,
    selectRemove,
    selectModify,
    selectDispatch,
    selectClear
};


void* selectInit() {
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    FD_ZERO(&data->readfds);
    FD_ZERO(&data->writefds);
    return data;
}

int selectAdd(struct Channel* channel, struct EventLoop* eventLoop) {
    if (channel->fd >= max_fds_size) return -1;
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    addFdSet(channel, eventLoop);
    return 0;
}

int selectRemove(struct Channel* channel, struct EventLoop* eventLoop) {
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    clrFdSet(channel, eventLoop);
    return 0;
}

int selectModify(struct Channel* channel, struct EventLoop* eventLoop) {
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    addFdSet(channel, eventLoop);
    clrFdSet(channel, eventLoop);
    return 0;
}

int selectDispatch(struct EventLoop* eventLoop, int timeout) {
    struct SelectData* data = (struct SelectData*)eventLoop->dispatcherData;
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = data->readfds;
    fd_set wrtmp = data->writefds;
    int count = select(max_fds_size, &rdtmp, &wrtmp, NULL, &val);
    errif_exit(count == -1, "select", true);
    for (int i = 0; i < max_fds_size; ++i) {
        if (FD_ISSET(i, &rdtmp)) {
            eventActivate(eventLoop, i, ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp)) {
            eventActivate(eventLoop, i, WriteEvent);
        }
    }
    return 0;
}

int selectClear(struct EventLoop* eventLoop) {
    struct SelectData* data = (struct SelectData*)eventLoop->dispatcherData;
    free(data);
}

void addFdSet(struct Channel* channel, struct EventLoop* eventLoop) {
    struct SelectData* data = (struct SelectData*)eventLoop->dispatcherData;
    if (channel->events & ReadEvent) FD_SET(channel->fd, &data->readfds);
    if (channel->events & WriteEvent) FD_SET(channel->fd, &data->writefds);
}

void clrFdSet(struct Channel* channel, struct EventLoop* eventLoop) {
    struct SelectData* data = (struct SelectData*)eventLoop->dispatcherData;
    if (!(channel->events & ReadEvent)) FD_CLR(channel->fd, &data->readfds);
    if (!(channel->events & WriteEvent)) FD_CLR(channel->fd, &data->writefds);
}
