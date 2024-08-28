#include "buffer.h"
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>

Buffer::Buffer(int size): m_capacity(size), m_readPos(0), m_writePos(0) {
    m_data = (char*) malloc(size);
    bzero(m_data, size);
}

Buffer::~Buffer() {
    if (m_data) {
        free(m_data);
    }
}

bool Buffer::sizeDetection(int size) {
    // 1. 内存够用, 不扩容
    if (writeableSize() >= size) return true;
    // 2. 内存需合并才够用, 不扩容 -> 剩余的可写 + 已读 >= capacity
    if (m_readPos + writeableSize() >= size) {
        int readable = readableSize();
        memcpy(m_data, m_data + m_readPos, readable);
        m_readPos = 0;
        m_writePos = readable;
    } else {
        // 3. 内存不够用, 扩容
        void* tmp = realloc(m_data, m_capacity + size);
        if (tmp == NULL) {
            printf("bufferSizeDetection realloc failed.\n");
            return false;
        }
        memset(tmp + m_capacity, 0, size);
        // 更新数据
        m_data = (char*)tmp;
        m_capacity += size;
    }
    return true;
}


bool Buffer::appendString(const char* data) {
    int size = strlen(data);
    return appendString(data, size);
}

bool Buffer::appendString(const char* data, int size) {
    if (data == nullptr || size <= 0) return false;
    if (sizeDetection(size) == false) return false;
    // 数据拷贝
    memcpy(m_data + m_writePos, data, size);
    m_writePos += size;
    return true;
}

int Buffer::socketRead(int fd) {
    // read/recv/readv
    struct iovec vec[2];
    // 初始化数组元素
    int writeable = writeableSize();
    vec[0].iov_base = m_data + m_writePos;
    vec[0].iov_len = writeable;
    vec[1].iov_base = (char*) malloc(40960);
    errif_exit(vec[1].iov_base == NULL, "bufferSocketRead");
    bzero(vec[1].iov_base, sizeof(vec[1].iov_base));
    vec[1].iov_len = 40960;
    // 接收数据
    int result = readv(fd, vec, 2);
    if (result == -1) return -1;
    if (result <= writeable) {
        // 未使用第二块内存
        m_writePos += result;
    } else {
        m_writePos = m_capacity;
        appendString((char*)vec[1].iov_base, result - writeable);
    }
    free(vec[1].iov_base);
    return result;
}

char* Buffer::findCRLF() {
    // strstr -> 大字符串中匹配子字符串, 但是遇到\0就结束
    // menmen -> 大数据块中匹配子数据块, 需要指定数据块大小
    char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
    return ptr;
}

int Buffer::sendData(int socket) {
    int readable = readableSize();
    if (readable > 0) {
        int count = send(socket, m_data + m_readPos, readable, 0);
        if (count > 0) {
            m_readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
