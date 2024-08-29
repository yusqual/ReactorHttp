#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <myheads/base.h>

class Buffer {
public:
    Buffer(int size);
    ~Buffer();

    // 根据所需size大小调整buffer内存空间, 空间不够会自动扩容
    bool sizeDetection(int size);
    // 获取剩余的可写的内存容量
    inline int writeableSize() { return m_capacity - m_writePos; }
    // 获取剩余的可读的内存容量
    inline int readableSize() { return m_writePos - m_readPos; }
    // 写内存 -> 1. 直接写  2. 接收套接字数据
    bool appendString(const char* data);
    bool appendString(const char* data, int size);
    int socketRead(int fd);
    // 根据\r\n取出一行, 找到其在数据块中的位置,返回该位置
    char* findCRLF();
    // 发送数据
    int sendData(int socket);

    // 得到读数据的起始位置
    inline char* data() { return m_data + m_readPos; }
    // 更改读位置
    inline int readPosIncrease(int count) { return (m_readPos += count); }

private:
    // 指向内存的指针
    char* m_data;
    int m_capacity;
    int m_readPos;
    int m_writePos;
};

#endif  // _BUFFER_H_