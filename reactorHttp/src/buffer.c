#include "buffer.h"

struct Buffer* bufferInit(int capacity) {
    struct Buffer* buffer = (struct Buffer*) malloc(sizeof(struct Buffer));
    errif_exit(buffer == NULL, "bufferInit_1", true);
    buffer->data = (char*) calloc(capacity, sizeof(char));
    errif_exit(buffer->data == NULL, "bufferInit_2", true);
    buffer->capacity = capacity;
    buffer->readPos = buffer->writePos = 0;
    return buffer;
}

void bufferDestroy(struct Buffer* buffer) {
    if (buffer) {
        if (buffer->data) {
            free(buffer->data);
            buffer->data = NULL;
        }
        free(buffer);
        buffer = NULL;
    }
}

bool bufferSizeDetection(struct Buffer* buffer, int size) {
    // 1. 内存够用, 不扩容
    if (bufferWriteableSize(buffer) >= size) return;
    // 2. 内存需合并才够用, 不扩容 -> 剩余的可写 + 已读 >= capacity
    if (buffer->readPos + bufferWriteableSize(buffer) >= size) {
        int readable = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readPos, readable);
        buffer->readPos = 0;
        buffer->writePos = readable;
    } else {
        // 3. 内存不够用, 扩容
        char* tmp = (char*) realloc(buffer->data, buffer->capacity + size);
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
