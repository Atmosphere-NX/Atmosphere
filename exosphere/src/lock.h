#ifndef EXOSPHERE_LOCK_H
#define EXOSPHERE_LOCK_H

#include <stdbool.h>

/* Simple atomics driver for Exosphere. */

/* Acquire a lock. */
static inline void lock_acquire(bool *l) {
    while (__atomic_test_and_set(l, __ATOMIC_ACQUIRE)) {
        /* Wait to acquire lock. */
    }
}

/* Release a lock. */
static inline void lock_release(bool *l) {
    __atomic_clear(l, __ATOMIC_RELEASE);
}

/* Try to acquire a lock. */
static inline bool lock_try_acquire(bool *l) {
    return __atomic_test_and_set(l, __ATOMIC_ACQUIRE);
}

#endif