#include "dispatcher.h"
#include <sys/epoll.h>

static void* epollInit();
static bool epollAdd(struct Channel* channel, struct EventLoop* eventLoop);
static bool epollRemove(struct Channel* channel, struct EventLoop* eventLoop);
static bool epollModify(struct Channel* channel, struct EventLoop* eventLoop);
static bool epollDispatch(struct EventLoop* eventLoop, int timeout);
static bool epollClear(struct EventLoop* eventLoop);

static bool epollCtl(struct Channel* channel, struct EventLoop* eventLoop, int op);

const int max_evs_size = 520;

// epoll需要的数据块
struct EpollData {
    int epfd;
    struct epoll_event* events;
};
// epoll对应的dispatcher
struct Dispatcher epollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear
};


void* epollInit() {
    struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
    errif_exit(data == NULL, "epollInit_1", true);
    data->epfd = epoll_create(1);
    errif_exit(data->epfd == -1, "epollInit_epoll_create", true);
    data->events = (struct epoll_event*)calloc(max_evs_size, sizeof(struct epoll_event));
    errif_exit(data->events == NULL, "epollInit_2", true);
    return data;
}

bool epollAdd(struct Channel* channel, struct EventLoop* eventLoop) {
    bool ret = epollCtl(channel, eventLoop, EPOLL_CTL_ADD);
    errif_exit(ret == false, "epoll_ctl_add", false);
    return ret;
}

bool epollRemove(struct Channel* channel, struct EventLoop* eventLoop) {
    bool ret = epollCtl(channel, eventLoop, EPOLL_CTL_DEL);
    errif_exit(ret == false, "epoll_ctl_del", false);
    return ret;
}

bool epollModify(struct Channel* channel, struct EventLoop* eventLoop) {
    bool ret = epollCtl(channel, eventLoop, EPOLL_CTL_MOD);
    errif_exit(ret == false, "epoll_ctl_mod", false);
    return ret;
}

bool epollDispatch(struct EventLoop* eventLoop, int timeout) {
    struct EpollData* data = (struct EpollData*)eventLoop->dispatcherData;
    int count = epoll_wait(data->epfd, data->events, max_evs_size, timeout);
    for (int i = 0; i < count; ++i) {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP) {
            // 对方断开了连接
            // epollRemove();
            continue;
        }
        if (events & EPOLLIN) { // 读事件
            eventActivate(eventLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT) { // 写事件
            eventActivate(eventLoop, fd, WriteEvent);
        }
    }
    return true;
}

bool epollClear(struct EventLoop* eventLoop) {
    struct EpollData* data = (struct EpollData*)eventLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return true;
}

bool epollCtl(struct Channel* channel, struct EventLoop* eventLoop, int op) {
    struct EpollData* data = (struct EpollData*)eventLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd;
    // ev.events = channel->events; // 两者events不同,不可直接赋值.channel中的events是我们自己定义的数值
    ev.events = 0;
    if (channel->events & ReadEvent) ev.events |= EPOLLIN;
    if (channel->events & WriteEvent) ev.events |= EPOLLOUT;
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret == 0;
}
