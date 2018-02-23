#ifndef EXOSPHERE_BPMPFW_TIMER_H
#define EXOSPHERE_BPMPFW_TIMER_H

#define TIMERUS_CNTR_1US_0 (*((volatile uint32_t *)(0x60005010)))

static inline void timer_wait(uint32_t microseconds) {
    uint32_t old_time = TIMERUS_CNTR_1US_0;
    while (TIMERUS_CNTR_1US_0 - old_time <= microseconds) {
        /* Spin-lock. */
    }
}

void spinlock_wait(uint32_t count);

#endif