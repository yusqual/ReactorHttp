#ifndef _CHANNEL_MAP_H_
#define _CHANNEL_MAP_H_

#include "channel.h"

struct ChannelMap {
    int size;   // 数组大小
    struct Channel** list;
};

// 初始化
struct ChannelMap* channelMapInit(int size);

// 清空map
void channelMapClear(struct ChannelMap* map);

// 重新分配内存空间, unitSize为每个元素大小, 默认为指针大小
bool channelMapResize(struct ChannelMap* map, int newSize, int unitSize);


#endif // _CHANNEL_MAP_H_