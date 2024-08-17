#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdbool.h>
#define _C_PROJECT_
#include <myheads/base.h>

// 定义回调函数函数指针
typedef int (*handleFunc)(void* arg);

// 定义文件描述符的读写事件
enum {
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};

// Channel结构体
struct Channel {
    // 文件描述符
    int fd;
    // 触发事件
    int events;
    // 回调函数
    handleFunc readFunc;
    handleFunc writeFunc;
    // 回调函数的参数
    void* arg;
};

// 初始化一个Channel
struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, void* arg);
// 修改fd的写事件 (检测 or 不检测)
void writeEventEnable(struct Channel* channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);

#endif  // _CHANNEL_H_