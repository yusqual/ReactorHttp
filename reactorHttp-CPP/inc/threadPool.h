#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "workerThread.h"
#include <vector>

class WorkerThread;

// 定义线程池
class ThreadPool {
public:
    ThreadPool(struct EventLoop* mainLoop, int threadSize);
    ~ThreadPool();
    // 启动线程池
    void run();
    // 取出线程池中某个子线程的反应堆实例
    EventLoop* takeWorkerEventLoop();
    // 获取主线程反应堆模型
    inline EventLoop* getMainLoop() { return m_mainLoop; }

private:
    // 主线程的反应堆模型
    EventLoop* m_mainLoop;
    bool m_isStart;
    int m_threadSize;  // 子线程数量
    std::vector<WorkerThread*> m_workerThreads;
    int m_index;  // 操作的线程号
};


#endif // _THREADPOOL_H_