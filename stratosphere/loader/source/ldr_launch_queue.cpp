#include "ldr_launch_queue.hpp"

#include <switch.h>

static launch_item_t g_launch_queue[LAUNCH_QUEUE_SIZE];

int launch_queue_get_index(u64 TID) {
    for(unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
        if(g_launch_queue[i].tid == TID) {
            return i;
        }
    }
    return LAUNCH_QUEUE_FULL;
}

int launch_queue_get_free_index(u64 TID) {
    for(unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
        if(g_launch_queue[i].tid == TID || g_launch_queue[i].tid == 0x0) {
            return i;
        }
    }
    return LAUNCH_QUEUE_FULL;
}

Result launch_queue_add(launch_item_t *item) {
    if(item->arg_size > LAUNCH_QUEUE_ARG_SIZE_MAX) {
        return 0x209;
    }
    
    int idx = launch_queue_get_free_index(item->tid);
    if(idx == LAUNCH_QUEUE_FULL)
        return 0x409;
    
    g_launch_queue[idx] = *item;
    return 0x0;
}

bool launch_queue_contains(uint64_t TID) {
    return launch_queue_get_index(TID) != LAUNCH_QUEUE_FULL;
}

void launch_queue_clear() {
    for (unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
        g_launch_queue[i].tid = 0;
    }
}