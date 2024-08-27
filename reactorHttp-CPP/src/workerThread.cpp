#include "workerThread.h"

WorkerThread::WorkerThread(int index): m_evLoop(nullptr), m_thread(nullptr), m_threadId(std::thread::id()), m_name("SubThread-" + std::to_string(index)) {
}

WorkerThread::~WorkerThread() {
    if (m_thread != nullptr) {
        delete m_thread;
    }
}

void WorkerThread::run(ThreadPool* pool) {
    // 创建子线程
    m_thread = new std::thread(&WorkerThread::subThreadRunning, this, pool);
    // 阻塞主线程, 保证subThreadRunning执行完毕,evloop被初始化完毕
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_evLoop == nullptr) {
        m_cond.wait(locker);
    }
}

void WorkerThread::subThreadRunning(ThreadPool* pool) {
    m_mutex.lock();
    m_evLoop = new EventLoop(m_name, pool);
    m_mutex.unlock();
    m_cond.notify_one();
    m_evLoop->run();
}
