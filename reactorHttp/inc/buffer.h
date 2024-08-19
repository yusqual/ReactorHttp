#ifndef _BUFFER_H_
#define _BUFFER_H_

#define _C_PROJECT_
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
// 写内存 -> 1. 直接写  2. 接收套接字数据
bool bufferAppendData(struct Buffer* buffer, const char* data, int size);
bool bufferAppendString(struct Buffer* buffer, const char* data);
int bufferSocketRead(struct Buffer* buffer, int fd);
// 根据\r\n取出一行, 找到其在数据块中的位置,返回该位置
char* bufferFindCRLF(struct Buffer* buffer);

#endif // _BUFFER_H_