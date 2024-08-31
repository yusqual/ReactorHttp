#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <myheads/base.h>
#include <functional>

// 定义回调函数函数指针
// typedef int (*handleFunc)(void* arg);
// using handleFunc = int (*)(void*);



// 定义文件描述符的读写事件
enum class FDEvent {
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};

// Channel结构体
class Channel {
public:
    // 可调用对象包装器, 得到的是地址, 还没有调用.该方法可以指向类的非静态成员函数
    using handleFunc = std::function<int(void*)>;
    // 构造 -> 初始化一个Channel
    Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);

    // 回调函数
    handleFunc readCallback;
    handleFunc writeCallback;
    handleFunc destroyCallback;

    // 修改fd的写事件 (检测 or 不检测)
    void modifyWriteEvent(bool flag);
    // 判断是否需要检测文件描述符的写事件
    bool isWriteEventEnable();

    // 私有成员
    inline int getEvent() { return m_events; }
    inline int getSocket() { return m_fd; }
    inline const void* getArg() { return m_arg; }

private:
    // 文件描述符
    int m_fd;
    // 触发事件
    int m_events;
    // 回调函数的参数
    void* m_arg;
};

#endif  // _CHANNEL_H_