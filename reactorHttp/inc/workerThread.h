#ifndef _WORKERTHREAD_H_
#define _WORKERTHREAD_H_

#include "eventLoop.h"

// 定义子线程对应的结构体
struct WorkerThread {
    pthread_t threadId;
    char name[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct EventLoop* evLoop;
};

// 初始化, index 为在线程池中的序号
int workerThreadInit(struct WorkerThread* thread, int index);
// 启动线程
void workerThreadRun(struct WorkerThread* thread);

#endif // _WORKERTHREAD_H_