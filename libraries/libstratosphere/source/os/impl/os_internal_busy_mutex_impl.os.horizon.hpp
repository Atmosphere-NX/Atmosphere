/*
 * Copyright (c) Atmosphère-NX
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
#pragma once
#include <stratosphere.hpp>

namespace ams::os::impl {

    namespace {

        ALWAYS_INLINE void PrefetchForBusyMutex(u32 *p) {
            /* Nintendo does PRFM pstl1keep. */
            __builtin_prefetch(p, 1);
        }

        ALWAYS_INLINE void WaitForEventsForBusyMutex() {
            __asm__ __volatile__("wfe" ::: "memory");
        }

        ALWAYS_INLINE u32 LoadExclusiveForBusyMutex(u32 *p) {
            u32 v;
            __asm__ __volatile__("ldaxr %w[v], %[p]" : [v]"=&r"(v) : [p]"Q"(*p) : "memory");
            return v;
        }

        ALWAYS_INLINE bool StoreExclusiveForBusyMutex(u32 *p, u32 v) {
            int result;
            __asm__ __volatile__("stxr %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory");
            return result == 0;
        }

        ALWAYS_INLINE void ClearExclusiveForBusyMutex() {
            __asm__ __volatile__("clrex" ::: "memory");
        }

        ALWAYS_INLINE void StoreUnlockValueForBusyMutex(u32 *p) {
            __asm__ __volatile__("stlr wzr, %[p]" :: [p]"Q"(*p) : "memory");
        }

    }

    void InternalBusyMutexImpl::Lock() {
        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Determine disable counters. */
        const auto cur_dc = tlr->disable_count;
        AMS_ASSERT(cur_dc < std::numeric_limits<decltype(cur_dc)>::max());
        const auto next_dc = cur_dc + 1;

        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Pre-fetch the busy mutex. */
        PrefetchForBusyMutex(p);

        /* Acquire the busy mutex. */
        while (true) {
            /* Set the updated disable counter. */
            tlr->disable_count = next_dc;

            /* Try to acquire. */
            const u32 v = LoadExclusiveForBusyMutex(p);
            if (AMS_LIKELY(v == 0) && AMS_LIKELY(StoreExclusiveForBusyMutex(p, 1))) {
                break;
            }

            /* Reset the disable counter, since we failed to acquire. */
            tlr->disable_count = cur_dc;

            /* If we don't hold any other busy mutexes, acknowledge any interrupts that occurred while we tried to acquire the lock. */
            if (cur_dc == 0 && tlr->interrupt_flag) {
                svc::SynchronizePreemptionState();
            }

            /* If the lock is held by another core, wait for it to be released. */
            if (v != 0) {
                WaitForEventsForBusyMutex();
            }
        }
    }

    bool InternalBusyMutexImpl::TryLock() {
        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Determine disable counters. */
        const auto cur_dc = tlr->disable_count;
        AMS_ASSERT(cur_dc < std::numeric_limits<decltype(cur_dc)>::max());
        const auto next_dc = cur_dc + 1;

        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Pre-fetch the busy mutex. */
        PrefetchForBusyMutex(p);

        /* Try to acquire the busy mutex. */
        while (true) {
            /* Set the updated disable counter. */
            tlr->disable_count = next_dc;

            /* Ensure we do whatever cleanup we need to. */
            auto release_guard = SCOPE_GUARD {
                /* Reset disable counter. */
                tlr->disable_count = cur_dc;

                /* If we don't hold any other busy mutexes, acknowledge any interrupts that occurred while we tried to acquire the lock. */
                if (cur_dc == 0 && tlr->interrupt_flag) {
                    svc::SynchronizePreemptionState();
                }
            };

            /* Try to acquire. */
            const u32 v = LoadExclusiveForBusyMutex(p);
            if (AMS_UNLIKELY(v != 0)) {
                ClearExclusiveForBusyMutex();
                return false;
            }

            if (AMS_LIKELY(StoreExclusiveForBusyMutex(p, 1))) {
                /* We successfully acquired the busy mutex. */
                release_guard.Cancel();
                return true;
            }
        }
    }

    void InternalBusyMutexImpl::Unlock() {
        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Unlock the mutex. */
        StoreUnlockValueForBusyMutex(p);

        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Determine disable counters. */
        const auto cur_dc = tlr->disable_count;
        AMS_ASSERT(cur_dc != 0);
        const auto next_dc = cur_dc - 1;

        /* Decrement disable count. */
        tlr->disable_count = next_dc;

        /* If we don't hold any other busy mutexes, acknowledge any interrupts that occurred while we held the lock. */
        if (next_dc == 0 && tlr->interrupt_flag) {
            svc::SynchronizePreemptionState();
        }
    }

}
