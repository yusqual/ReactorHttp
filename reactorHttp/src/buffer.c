#include "buffer.h"
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>

struct Buffer* bufferInit(int capacity) {
    struct Buffer* buffer = (struct Buffer*) malloc(sizeof(struct Buffer));
    errif_exit(buffer == NULL, "bufferInit_1", true);
    buffer->data = (char*) malloc(capacity);
    bzero(buffer->data, capacity);
    errif_exit(buffer->data == NULL, "bufferInit_2", true);
    buffer->capacity = capacity;
    buffer->readPos = buffer->writePos = 0;
    return buffer;
}

void bufferDestroy(struct Buffer* buffer) {
    if (buffer) {
        if (buffer->data) {
            free(buffer->data);
        }
        free(buffer);
    }
}

bool bufferSizeDetection(struct Buffer* buffer, int size) {
    // 1. 内存够用, 不扩容
    if (bufferWriteableSize(buffer) >= size) return true;
    // 2. 内存需合并才够用, 不扩容 -> 剩余的可写 + 已读 >= capacity
    if (buffer->readPos + bufferWriteableSize(buffer) >= size) {
        int readable = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readPos, readable);
        buffer->readPos = 0;
        buffer->writePos = readable;
    } else {
        // 3. 内存不够用, 扩容
        void* tmp = realloc(buffer->data, buffer->capacity + size);
        if (tmp == NULL) {
            printf("bufferSizeDetection realloc failed.\n");
            return false;
        }
        memset(tmp + buffer->capacity, 0, size);
        // 更新数据
        buffer->data = tmp;
        buffer->capacity += size;
    }
    return true;
}

int bufferWriteableSize(struct Buffer* buffer) {
    return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(struct Buffer* buffer) {
    return buffer->writePos - buffer->readPos;
}

bool bufferAppendData(struct Buffer* buffer, const char* data, int size) {
    if (buffer == NULL || data == NULL || data <= 0) return false;
    if (bufferSizeDetection(buffer, size) == false) return false;
    // 数据拷贝
    memcpy(buffer->data+buffer->writePos, data, size);
    buffer->writePos += size;
    return true;
}

bool bufferAppendString(struct Buffer* buffer, const char* data) {
    int size = strlen(data);
    return bufferAppendData(buffer, data, size);
}

int bufferSocketRead(struct Buffer* buffer, int fd) {
    // read/recv/readv
    struct iovec vec[2];
    // 初始化数组元素
    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writePos;
    vec[0].iov_len = writeable;
    vec[1].iov_base = (char*)calloc(40960, 1);
    errif_exit(vec[1].iov_base == NULL, "bufferSocketRead", true);
    vec[1].iov_len = 40960;
    // 接收数据
    int result = readv(fd, vec, 2);
    if (result == -1) return -1;
    if (result <= writeable) {
        // 未使用第二块内存
        buffer->writePos += result;
    } else {
        buffer->writePos = buffer->capacity;
        bufferAppendData(buffer, vec[1].iov_base, result - writeable);
    }
    free(vec[1].iov_base);
    return result;
}

char* bufferFindCRLF(struct Buffer* buffer) {
    // strstr -> 大字符串中匹配子字符串, 但是遇到\0就结束
    // menmen -> 大数据块中匹配子数据块, 需要指定数据块大小
    char* ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
    return ptr;
}

int bufferSendData(struct Buffer* buffer, int socket) {
    int readable = bufferReadableSize(buffer);
    if (readable > 0) {
        int count = send(socket, buffer->data + buffer->readPos, readable, 0);
        if (count > 0) {
            buffer->readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
