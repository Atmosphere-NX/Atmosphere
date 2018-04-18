#include "ldr_launch_queue.hpp"

#include <switch.h>

namespace LaunchQueue {
    static LaunchItem g_launch_queue[LAUNCH_QUEUE_SIZE];
        
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