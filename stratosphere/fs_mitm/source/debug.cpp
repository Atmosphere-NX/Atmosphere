#include <switch.h>
#include <cstring>
#include "debug.hpp"

static u64 g_num_logged = 0;

#define MAX_LOGS U64_MAX

void Reboot() {
    while (1) {
        /* ... */
    }
}

void Log(const void *data, int size) {
    /* ... */
}