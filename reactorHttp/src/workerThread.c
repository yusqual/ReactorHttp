#include "workerThread.h"

int workerThreadInit(struct WorkerThread* thread, int index) {
    thread->evLoop = NULL;
    thread->threadId = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}

void* subThreadRunning(void* arg) {
    struct WorkerThread* thread = (struct WorkerThread*)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    eventLoopRun(thread->evLoop);
    return NULL;
}

void workerThreadRun(struct WorkerThread* thread) {
    // 创建子线程
    pthread_create(&thread->threadId, NULL, subThreadRunning, thread);
    // 阻塞主线程, 保证subThreadRunning执行完毕,evloop被初始化完毕
    pthread_mutex_lock(&thread->mutex);
    while (thread->evLoop == NULL) {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}
