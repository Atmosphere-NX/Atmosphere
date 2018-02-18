#include "timers.h"

volatile void *g_timer_registers = NULL;

void set_timer_address(void *timer_base) {
    g_timer_registers = timer_base;
}

inline void *get_timer_address(void) {
    return g_timer_registers;
}

void wait(uint32_t microseconds) {
    uint32_t old_time = TIMERUS_CNTR_1US_0;
    while (TIMERUS_CNTR_1US_0 - old_time <= result) {
        /* Spin-lock. */
    }
}