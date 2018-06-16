#include <switch.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include "ldr_launch_queue.hpp"
#include "meta_tools.hpp"

static std::array<LaunchQueue::LaunchItem, LAUNCH_QUEUE_SIZE> g_launch_queue = {0};

Result LaunchQueue::add(u64 tid, const char *args, u64 arg_size) {
    if(arg_size > LAUNCH_QUEUE_ARG_SIZE_MAX) {
        return 0x209;
    }
    
    int idx = get_free_index(tid);
    if(idx == LAUNCH_QUEUE_FULL)
        return 0x409;
    
    g_launch_queue[idx].tid = tid;
    g_launch_queue[idx].arg_size = arg_size;
    
    std::copy(args, args + arg_size, g_launch_queue[idx].args);
    return 0x0;
}

Result LaunchQueue::add_copy(u64 tid_base, u64 tid) {
    int idx = get_index(tid_base);
    if (idx == LAUNCH_QUEUE_FULL) {
        return 0x0;
    }
    
    return add(tid, g_launch_queue[idx].args, g_launch_queue[idx].arg_size);
}


Result LaunchQueue::add_item(const LaunchItem *item) {
    if(item->arg_size > LAUNCH_QUEUE_ARG_SIZE_MAX) {
        return 0x209;
    }
    
    int idx = get_free_index(item->tid);
    if(idx == LAUNCH_QUEUE_FULL)
        return 0x409;
    
    g_launch_queue[idx] = *item;
    return 0x0;
}

int LaunchQueue::get_index(u64 tid) {
    auto it = std::find_if(g_launch_queue.begin(), g_launch_queue.end(), member_equals_fn(&LaunchQueue::LaunchItem::tid, tid));
    if (it == g_launch_queue.end()) {
        return LAUNCH_QUEUE_FULL;
    }
    return std::distance(g_launch_queue.begin(), it);
}

int LaunchQueue::get_free_index(u64 tid) {
    for(unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
        if(g_launch_queue[i].tid == tid || g_launch_queue[i].tid == 0x0) {
            return i;
        }
    }
    return LAUNCH_QUEUE_FULL;
}

bool LaunchQueue::contains(u64 tid) {
    return get_index(tid) != LAUNCH_QUEUE_FULL;
}

void LaunchQueue::clear() {
    for (unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
        g_launch_queue[i].tid = 0;
    }
}


LaunchQueue::LaunchItem *LaunchQueue::get_item(u64 tid) {
    int idx = get_index(tid);
    if (idx == LAUNCH_QUEUE_FULL) {
        return NULL;
    }
    return &g_launch_queue[idx];
}
