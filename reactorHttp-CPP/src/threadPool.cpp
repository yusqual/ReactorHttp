#include "threadPool.h"
#include <assert.h>

ThreadPool::ThreadPool(EventLoop* mainLoop, int threadSize): m_index(0), m_isStart(false), m_mainLoop(mainLoop), m_threadSize(threadSize) {
}

ThreadPool::~ThreadPool() {
    for (auto item : m_workerThreads) {
        delete item;
    }
}

void ThreadPool::run() {
    assert(!m_isStart);
    errif_exit(m_mainLoop->getThreadId() != std::this_thread::get_id(), "threadPoolRun");  // 操作的线程必须是主线程
    m_isStart = true;
    if (m_threadSize > 0) {
        for (int i = 0; i < m_threadSize; ++i) {
            // 初始化子线程
            WorkerThread* subThread = new WorkerThread(i);
            subThread->run(this);
            m_workerThreads.push_back(subThread);
        }
    }
}

EventLoop* ThreadPool::takeWorkerEventLoop() {
    assert(m_isStart);
    errif_exit(m_mainLoop->getThreadId() != std::this_thread::get_id(), "takeWorkerEventLoop");  // 操作的线程必须是主线程
    // 从线程池中找一个子线程,然后取出里面的反应堆实例
    EventLoop* evLoop = m_mainLoop;
    if (m_threadSize > 0) {
        evLoop = m_workerThreads[m_index]->getEventLoop();
        m_index = (m_index + 1) % m_threadSize;
    }
    return evLoop;
}


