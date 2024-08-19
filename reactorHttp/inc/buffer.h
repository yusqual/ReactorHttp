#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <myheads/base.h>

struct Buffer {
    // 指向内存的指针
    char* data;
    int capacity;
    int readPos;
    int writePos;
};

// init
struct Buffer* bufferInit(int capacity);
// destroy
void bufferDestroy(struct Buffer* buffer);
// 根据所需size大小调整buffer内存空间, 空间不够会自动扩容
bool bufferSizeDetection(struct Buffer* buffer, int size);
// 获取剩余的可写的内存容量
int bufferWriteableSize(struct Buffer* buffer);
// 获取剩余的可读的内存容量
int bufferReadableSize(struct Buffer* buffer);

#endif // _BUFFER_H_