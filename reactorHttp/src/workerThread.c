#include "workerThread.h"

struct threadInfo {
    struct WorkerThread* thread;
    struct ThreadPool* pool;
};

bool workerThreadInit(struct WorkerThread* thread, int index) {
    thread->evLoop = NULL;
    thread->threadId = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return true;
}

void* subThreadRunning(void* arg) {
    struct threadInfo* tInfo = (struct threadInfo*)arg;
    struct WorkerThread* thread = tInfo->thread;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name, tInfo->pool);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    eventLoopRun(thread->evLoop);
    return NULL;
}

void workerThreadRun(struct WorkerThread* thread, struct ThreadPool* pool) {
    struct threadInfo* tInfo = (struct threadInfo*)malloc(sizeof(struct threadInfo));
    tInfo->thread = thread;
    tInfo->pool = pool;
    // 创建子线程
    int res = pthread_create(&thread->threadId, NULL, subThreadRunning, tInfo);
    errif_exit(res != 0, "workerThreadRun", true);
    // 阻塞主线程, 保证subThreadRunning执行完毕,evloop被初始化完毕
    pthread_mutex_lock(&thread->mutex);
    while (thread->evLoop == NULL) {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}
