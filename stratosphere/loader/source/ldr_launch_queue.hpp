#pragma once
#include <switch.h>

#define LAUNCH_QUEUE_SIZE (10)
#define LAUNCH_QUEUE_FULL (-1)

#define LAUNCH_QUEUE_ARG_SIZE_MAX (0x8000)

namespace LaunchQueue { 
    struct LaunchItem {
        u64 tid;
        u64 arg_size;
        char args[LAUNCH_QUEUE_ARG_SIZE_MAX];
    };
    
    Result add(u64 tid, const char *args, u64 arg_size);
    Result add_item(const LaunchItem *item);
    int get_index(u64 tid);
    int get_free_index(u64 tid);
    bool contains(u64 tid);
    void clear();
}