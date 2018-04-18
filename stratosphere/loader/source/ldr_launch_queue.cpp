#include <switch.h>
#include <algorithm>

#include "ldr_launch_queue.hpp"


namespace LaunchQueue {
    static LaunchItem g_launch_queue[LAUNCH_QUEUE_SIZE];
    
    Result add(u64 tid, const char *args, u64 arg_size) {
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
    
    Result add_item(const LaunchItem *item) {
        if(item->arg_size > LAUNCH_QUEUE_ARG_SIZE_MAX) {
            return 0x209;
        }
        
        int idx = get_free_index(item->tid);
        if(idx == LAUNCH_QUEUE_FULL)
            return 0x409;
        
        g_launch_queue[idx] = *item;
        return 0x0;
    }
    
    int get_index(u64 tid) {
        for(unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
            if(g_launch_queue[i].tid == tid) {
                return i;
            }
        }
        return LAUNCH_QUEUE_FULL;
    }
    
    int get_free_index(u64 tid) {
        for(unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
            if(g_launch_queue[i].tid == tid || g_launch_queue[i].tid == 0x0) {
                return i;
            }
        }
        return LAUNCH_QUEUE_FULL;
    }
    
    bool contains(u64 tid) {
        return get_index(tid) != LAUNCH_QUEUE_FULL;
    }
    
    void clear() {
        for (unsigned int i = 0; i < LAUNCH_QUEUE_SIZE; i++) {
            g_launch_queue[i].tid = 0;
        }
    }
}