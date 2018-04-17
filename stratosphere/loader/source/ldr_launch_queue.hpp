#pragma once
#include <switch.h>

#define LAUNCH_QUEUE_SIZE (10)
#define LAUNCH_QUEUE_FULL (-1)

#define LAUNCH_QUEUE_ARG_SIZE_MAX (0x8000)

typedef struct launch_item_t {
    u64 tid;
    u64 arg_size;
    char args[LAUNCH_QUEUE_ARG_SIZE_MAX];
} launch_item_t;

Result launch_queue_add(launch_item_t item);
int launch_queue_get_index(u64 TID);
int launch_queue_get_free_index(u64 TID = 0);
bool launch_queue_contains(u64 TID);
void launch_queue_clear();