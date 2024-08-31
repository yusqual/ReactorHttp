#ifndef _WORKERTHREAD_H_
#define _WORKERTHREAD_H_

#include "eventLoop.h"
#include "threadPool.h"
#include <thread>
#include <condition_variable>
class ThreadPool;
class EventLoop;

// 定义子线程对应的结构体
class WorkerThread {
public:
    WorkerThread(int index);
    ~WorkerThread();

    // 启动线程
    void run(struct ThreadPool* pool);

    inline EventLoop* getEventLoop() { return m_evLoop; }

private:
    void subThreadRunning(ThreadPool* pool);

private:
    std::thread* m_thread;
    std::thread::id m_threadId;
    std::string m_name;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    EventLoop* m_evLoop;
};

#endif  // _WORKERTHREAD_H_