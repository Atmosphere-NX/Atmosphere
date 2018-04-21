#pragma once
#include <switch.h>

#define LAUNCH_QUEUE_SIZE (10)
#define LAUNCH_QUEUE_FULL (-1)

#define LAUNCH_QUEUE_ARG_SIZE_MAX (0x8000)

class LaunchQueue { 
    public:
        struct LaunchItem {
            u64 tid;
            u64 arg_size;
            char args[LAUNCH_QUEUE_ARG_SIZE_MAX];
        };
        
        static Result add(u64 tid, const char *args, u64 arg_size);
        static Result add_item(const LaunchItem *item);
        static Result add_copy(u64 tid_base, u64 new_tid);
        static int get_index(u64 tid);
        static int get_free_index(u64 tid);
        static bool contains(u64 tid);
        static void clear();
};