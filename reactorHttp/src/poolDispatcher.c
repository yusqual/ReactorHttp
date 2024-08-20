#include "dispatcher.h"
#include <sys/poll.h>

static void* pollInit();
static bool pollAdd(struct Channel* channel, struct EventLoop* eventLoop);
static bool pollRemove(struct Channel* channel, struct EventLoop* eventLoop);
static bool pollModify(struct Channel* channel, struct EventLoop* eventLoop);
static bool pollDispatch(struct EventLoop* eventLoop, int timeout);
static bool pollClear(struct EventLoop* eventLoop);

#define max_fds_size 520

// poll需要的数据块
struct PollData {
    int maxfd;
    struct pollfd fds[max_fds_size];
};
// poll对应的dispatcher
struct Dispatcher pollDispatcher = {
    pollInit,
    pollAdd,
    pollRemove,
    pollModify,
    pollDispatch,
    pollClear
};


void* pollInit() {
    struct PollData* data = (struct PollData*)malloc(sizeof(struct PollData));
    errif_exit(data == NULL, "pollInit", true);
    data->maxfd = 0;
    for (int i = 0; i < max_fds_size; ++i) {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
}

bool pollAdd(struct Channel* channel, struct EventLoop* eventLoop) {
    struct PollData* data = (struct PollData*)eventLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent) events |= POLLIN;
    if (channel->events & WriteEvent) events |= POLLOUT;
    for (int i = 0; i < max_fds_size; ++i) {
        if (data->fds[i].fd == -1) {
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->maxfd = data->maxfd < i ? i : data->maxfd;
            return true;
        }
    }
    return false;
}

bool pollRemove(struct Channel* channel, struct EventLoop* eventLoop) {
    struct PollData* data = (struct PollData*)eventLoop->dispatcherData;
    for (int i = 0; i < max_fds_size; ++i) {
        if (data->fds[i].fd == channel->fd) {
            data->fds[i].fd = -1;
            data->fds[i].events = 0;
            data->fds[i].revents = 0;
            return true;
        }
    }
    return false;
}

bool pollModify(struct Channel* channel, struct EventLoop* eventLoop) {
    struct PollData* data = (struct PollData*)eventLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent) events |= POLLIN;
    if (channel->events & WriteEvent) events |= POLLOUT;
    for (int i = 0; i < max_fds_size; ++i) {
        if (data->fds[i].fd == channel->fd) {
            data->fds[i].events = events;
            return true;
        }
    }
    return false;
}

bool pollDispatch(struct EventLoop* eventLoop, int timeout) {
    struct PollData* data = (struct PollData*)eventLoop->dispatcherData;
    int count = poll(data->fds, data->maxfd+1, timeout);
    errif_exit(count == -1, "poll", true);
    for (int i = 0, done = 0; i <= data->maxfd && done < count; ++i) {
        if (data->fds[i].fd == -1) continue;
        if (data->fds[i].revents & POLLIN) {
            eventActivate(eventLoop, data->fds[i].fd, ReadEvent);
        }
        if (data->fds[i].revents & POLLOUT) {
            eventActivate(eventLoop, data->fds[i].fd, WriteEvent);
        }
        ++done;
    }
    return true;
}

bool pollClear(struct EventLoop* eventLoop) {
    struct PollData* data = (struct PollData*)eventLoop->dispatcherData;
    free(data);
    return true;
}

