#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "workerThread.h"

struct WorkerThread;

// 定义线程池
struct ThreadPool {
    // 主线程的反应堆模型
    struct EventLoop* mainLoop;
    bool isStart;
    int threadNum;  // 子线程数量
    struct WorkerThread* workerThreads;
    int index;  // 操作的线程号
};

// 初始化线程池
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int threadNum);
// 启动线程池
void threadPoolRun(struct ThreadPool* pool);
// 取出线程池中某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);
#endif // _THREADPOOL_H_