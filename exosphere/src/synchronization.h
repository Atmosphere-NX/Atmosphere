/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef EXOSPHERE_SYNCHRONIZATION_H
#define EXOSPHERE_SYNCHRONIZATION_H

#include <stdatomic.h>
#include "utils.h"

/* Simple atomics driver for Exosphere. */

typedef struct {
    struct {
        uint8_t ticket_number   : 7;
        uint8_t is_entering     : 1;
    } customers[4];
} critical_section_t;

static inline void __dsb_sy(void) {
    __asm__ __volatile__ ("dsb sy" ::: "memory");
}

static inline void __dsb_ish(void) {
    __asm__ __volatile__ ("dsb ish" ::: "memory");
}

static inline void __dmb_sy(void) {
    __asm__ __volatile__ ("dmb sy" ::: "memory");
}

static inline void __isb(void) {
    __asm__ __volatile__ ("isb" ::: "memory");
}

static inline void __sev(void) {
    __asm__ __volatile__ ("sev");
}

static inline void __sevl(void) {
    __asm__ __volatile__ ("sevl");
}

static inline void __wfe(void) {
    __asm__ __volatile__ ("wfe");
}

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
    return !atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
}

/*
    Enter a critical section, using the Lamport's bakery algorithm.
    https://en.wikipedia.org/wiki/Lamport%27s_bakery_algorithm

    This is invoked on warmboot before the MMU is turned on, therefore
    exclusive load/store instructions can't be used.

    Note: Nintendo has tried to implement that algorithm, but it seems that
    they didn't understand how it works, and their implementation
    is therefore a complete failure: in particular, the "ticket number" is
    always the same (core0 will always enter the critical section before all the cores,
    and so on...), and thus there can be starvation, etc.

    Nintendo, wtf.
*/
ALINLINE static inline unsigned int critical_section_enter(volatile critical_section_t *critical_section) {
    unsigned int id = get_core_id();
    uint8_t my_ticket_number = 0;
    uint8_t tmp;

    critical_section->customers[id].is_entering = 1;
    for (unsigned int i = 0; i < 4; i++) {
        tmp = critical_section->customers[id].ticket_number;
        my_ticket_number = tmp > my_ticket_number ? tmp : my_ticket_number;
    }

    critical_section->customers[id].ticket_number = ++my_ticket_number;
    critical_section->customers[id].is_entering = 0;
    __dsb_sy();
    __sev();

    for (unsigned int i = 0; i < 4; i++) {
        __sevl();
        do {
            __wfe();
        } while (critical_section->customers[i].is_entering);
 
        __sevl();
        do {
            __wfe();
            tmp = critical_section->customers[i].ticket_number;
        } while (tmp && (tmp < my_ticket_number || (tmp == my_ticket_number && i < id)));
    }

    __dmb_sy();
    return id;
}

/*
    Leaves a critical section, using the Lamport's bakery algorithm (see above).

    Note: Nintendo failed even that: they're clearing the entire critical section state
    instead of just the counter associated to the current core.

    Nintendo, wtf.
*/
ALINLINE static inline void critical_section_leave(volatile critical_section_t *critical_section) {
    critical_section->customers[get_core_id()].ticket_number = 0;
    __dsb_sy();
    __sev();
}

#endif
