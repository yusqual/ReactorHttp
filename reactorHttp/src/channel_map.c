#include "channelMap.h"

struct ChannelMap* channelMapInit(int size) {
    struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
    map->list = (struct Channel**)malloc(size * sizeof(struct Channel*));
    return map;
}

void channelMapClear(struct ChannelMap* map) {
    if (map != NULL) {
        for (int i = 0; i < map->size; ++i) {
            if (map->list[i] != NULL) free(map->list[i]);
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}

bool channelMapResize(struct ChannelMap* map, int newSize, int unitSize) {
    if (map->size < newSize) {
        struct Channel** tmp = realloc(map->list, newSize* unitSize);
        if (tmp == NULL) return false;
        map->list = tmp;
        memset(&map->list[map->size], 0, (newSize - map->size) * unitSize);
        map->size = newSize;
    }
    return true;
}
