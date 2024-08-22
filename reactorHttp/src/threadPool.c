#include "threadPool.h"
#include <assert.h>

struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int threadNum) {
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    errif_exit(pool == NULL, "threadPoolInit_1", true);
    pool->index = 0;
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->threadNum = threadNum;
    pool->workerThreads = (struct WorkerThread*)malloc(threadNum * sizeof(struct WorkerThread));
    errif_exit(pool->workerThreads == NULL, "threadPoolInit_2", true);
    return pool;
}

void threadPoolRun(struct ThreadPool* pool) {
    assert(pool && !pool->isStart);
    // errif_exit(pool == NULL && !pool->isStart, "threadPoolRun_1", true);
    errif_exit(pool->mainLoop->threadId != pthread_self(), "threadPoolRun_2", true);    // 操作的线程必须是主线程
    pool->isStart = true;
    if (pool->threadNum) {
        for (int i = 0; i < pool->threadNum; ++i) {
            // 初始化子线程
            workerThreadInit(&pool->workerThreads[i], i);
            workerThreadRun(&pool->workerThreads[i], pool);
        }
    }
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool) {
    errif_exit(!pool->isStart, "takeWorkerEventLoop_1", true);
    errif_exit(pool->mainLoop->threadId != pthread_self(), "takeWorkerEventLoop_2", true);    // 操作的线程必须是主线程
    // 从线程池中找一个子线程,然后取出里面的反应堆实例
    struct EventLoop* evLoop = pool->mainLoop;
    if (pool->threadNum > 0) {
        evLoop = pool->workerThreads[pool->index].evLoop;
        pool->index = (pool->index + 1) % pool->threadNum;
    }
    return evLoop;
}
