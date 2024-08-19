#include "eventLoop.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

// 写数据
void taskWakeup(struct EventLoop* evLoop) {
    const char* msg = "Get up!";
    write(evLoop->socketPair[0], msg, strlen(msg));
}

// 读数据
int readLocalMsg(void* arg) {
    struct EventLoop* evLoop = (struct EventLoop*) arg;
    char buf[256];
    int res = read(evLoop->socketPair[1], buf, sizeof(buf));
    errif_exit(res == -1, "readLocalMsg", true);
    return res;
}

struct EventLoop* eventLoopInit() {
    return eventLoopInitEx("MainThread");
}

struct EventLoop* eventLoopInitEx(const char* threadName) {
    struct EventLoop* evLoop = (struct EventLoop*) malloc(sizeof(struct EventLoop));
    errif_exit(evLoop == NULL, "eventLoopInitEx_1", true);
    evLoop->isRunning = true;
    evLoop->threadId = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL);
    strcpy(evLoop->threadName, threadName);
    evLoop->dispatcher = &epollDispatcher;  // 使用epoll
    evLoop->dispatcherData = evLoop->dispatcher->init();
    evLoop->head = evLoop->tail = NULL;        // 链表
    evLoop->channelmap = channelMapInit(128);  // channelmap
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    errif_exit(ret == -1, "eventLoopInitEx_socketpair", true);
    // 指定evLoop->socketPair[0]发送数据,[1]接收数据
    // 添加[1]到dipatcher检测列表中, 设置读事件原因: epoll默认采用水平触发, 若不读取读缓冲区数据则会一直触发
    struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMsg, NULL, evLoop);
    eventLoopAddTask(evLoop, channel, ADD);  // 添加channel到任务队列
    return evLoop;
}

bool eventLoopRun(struct EventLoop* evLoop) {
    assert(evLoop != NULL);
    // 检测线程id是否正常
    if (evLoop->threadId != pthread_self()) return false;
    // 取出事件分发和检测模型
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    while (evLoop->isRunning) {
        dispatcher->dispatch(evLoop, 2000);  // 超时时长2s
        eventLoopProcessTask(evLoop);
    }
    return true;
}

bool eventActivate(struct EventLoop* evLoop, int fd, int event) {
    if (evLoop == NULL || fd < 0) return false;
    // 取出channel
    struct Channel* channel = evLoop->channelmap->list[fd];
    assert(channel->fd == fd);
    if (event & ReadEvent && channel->readCallback) channel->readCallback(channel->arg);
    if (event & WriteEvent && channel->writeCallback) channel->writeCallback(channel->arg);
    return true;
}

bool eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type) {
    // 加锁, 保护共享资源
    pthread_mutex_lock(&evLoop->mutex);
    // 创建新节点
    struct ChannelElement* node = (struct ChannelElement*) malloc(sizeof(struct ChannelElement));
    if (node == NULL) {
        printf("eventLoopAddTask malloc failed.\n");
        return false;
    }
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // 链表为空
    if (evLoop->head == NULL) {
        evLoop->head = evLoop->tail = node;
    } else {
        evLoop->tail->next = node;
        evLoop->tail = node;
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // 处理节点
    /*
     * 细节:
     *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
     *       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
     *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
     *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
     */
    if (evLoop->threadId == pthread_self()) {
        // 当前是子(主)线程, 处理任务队列中的任务
        eventLoopProcessTask(evLoop);
    } else {
        // 当前是主线程 -- 告诉子线程处理任务队列中的任务
        // 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
        // 通过在dispatcher检测的fd中添加一个单独的fd,来控制dispatcher检测函数能够直接返回
        taskWakeup(evLoop);
    }
    return true;
}

bool eventLoopProcessTask(struct EventLoop* evLoop) {
    pthread_mutex_lock(&evLoop->mutex);
    struct ChannelElement* head = evLoop->head;
    while (head) {
        struct Channel* channel = head->channel;
        if (head->type == ADD) {
            eventLoopAdd(evLoop, channel);
        } else if (head->type == DEL) {
            eventLoopDel(evLoop, channel);
        } else if (head->type == MOD) {
            eventLoopMod(evLoop, channel);
        }
        struct ChannelElement* tmp = head;
        head = head->next;
        free(tmp);
    }
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
    return true;
}

bool eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel) {
    int fd = channel->fd;
    struct ChannelMap* channelmap = evLoop->channelmap;
    if (fd >= channelmap->size) {
        // 扩容
        if (!channelMapResize(channelmap, fd, sizeof(struct Channel*))) return false;
    }
    // 找到fd对应的数组元素位置并存储
    if (channelmap->list[fd] == NULL) {
        channelmap->list[fd] = channel;
        return evLoop->dispatcher->add(channel, evLoop);
    }
    return false;
}

bool eventLoopDel(struct EventLoop* evLoop, struct Channel* channel) {
    int fd = channel->fd;
    struct ChannelMap* channelmap = evLoop->channelmap;
    if (fd >= channelmap->size) return false;
    // 找到fd对应的数组元素位置并存储
    return evLoop->dispatcher->remove(channel, evLoop);
}

bool eventLoopMod(struct EventLoop* evLoop, struct Channel* channel) {
    int fd = channel->fd;
    struct ChannelMap* channelmap = evLoop->channelmap;
    if (fd >= channelmap->size || channelmap->list[fd] == NULL) return false;
    return evLoop->dispatcher->modify(channel, evLoop);
}

bool destroyChannel(struct EventLoop* evLoop, struct Channel* channel) {
    evLoop->channelmap->list[channel->fd] = NULL;
    close(channel->fd);
    free(channel);
    channel = NULL;
    return true;
}
