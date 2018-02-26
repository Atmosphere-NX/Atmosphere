#ifndef EXOSPHERE_LOCK_H
#define EXOSPHERE_LOCK_H

#include <stdatomic.h>
#include <stdbool.h>

/* Simple atomics driver for Exosphere. */

/* Acquire a lock. */
static inline void lock_acquire(atomic_flag *flag) {
    while (atomic_flag_test_and_set_explicit(flag, memory_order_acquire)) {
        /* Wait to acquire lock. */
    }
}

/* Release a lock. */
static inline void lock_release(atomic_flag *flag) {
    atomic_flag_clear_explicit(flag, memory_order_release);
}

/* Try to acquire a lock. */
static inline bool lock_try_acquire(atomic_flag *flag) {
    return atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
}

#endif