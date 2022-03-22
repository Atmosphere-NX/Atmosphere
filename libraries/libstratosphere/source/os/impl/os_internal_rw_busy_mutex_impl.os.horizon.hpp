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
#include "os_disable_counter.os.horizon.hpp"

namespace ams::os::impl {

    namespace {

        ALWAYS_INLINE void PrefetchForBusyMutex(u32 *p) {
            /* Nintendo does PRFM pstl1keep. */
            __builtin_prefetch(p, 1);
        }

        ALWAYS_INLINE void SendEventLocalForBusyMutex() {
            __asm__ __volatile__("sevl" ::: "memory");
        }

        ALWAYS_INLINE void WaitForEventsForBusyMutex() {
            __asm__ __volatile__("wfe" ::: "memory");
        }

        ALWAYS_INLINE u32 LoadAcquireExclusiveForBusyMutex(u32 *p) {
            u32 v;
            __asm__ __volatile__("ldaxr %w[v], %[p]" : [v]"=&r"(v) : [p]"Q"(*p) : "memory");
            return v;
        }

        ALWAYS_INLINE u32 LoadExclusiveForBusyMutex(u32 *p) {
            u32 v;
            __asm__ __volatile__("ldxr %w[v], %[p]" : [v]"=&r"(v) : [p]"Q"(*p) : "memory");
            return v;
        }

        ALWAYS_INLINE bool StoreReleaseExclusiveForBusyMutex(u32 *p, u32 v) {
            int result;
            __asm__ __volatile__("stlxr %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory");
            return result == 0;
        }

        ALWAYS_INLINE bool StoreExclusiveForBusyMutex(u32 *p, u32 v) {
            int result;
            __asm__ __volatile__("stxr %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory");
            return result == 0;
        }

        ALWAYS_INLINE void StoreReleaseWriteLockValueForBusyMutex(u32 *p) {
            u8 * const p8 = InternalReaderWriterBusyMutexValue::GetWriterCurrentPointer(p);

            u8 v;
            __asm__ __volatile__("ldrb  %w[v], %[p8]\n"
                                 "add   %w[v], %w[v], #1\n"
                                 "stlrb %w[v], %[p8]\n"
                                 : [v]"=&r"(v)
                                 : [p8]"Q"(*p8)
                                 : "memory");
        }

    }

    void InternalReaderWriterBusyMutexImpl::AcquireReadLock() {
        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Determine disable counters. */
        const auto cur_dc = tlr->disable_count;
        AMS_ABORT_UNLESS(cur_dc < std::numeric_limits<decltype(cur_dc)>::max());
        const auto next_dc = cur_dc + 1;

        /* Check that we're allowed to use busy mutexes. */
        CallCheckBusyMutexPermission();

        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Pre-fetch the busy mutex. */
        PrefetchForBusyMutex(p);

        /* Acquire the read-lock for the mutex. */
        while (true) {
            /* Set the updated disable counter. */
            tlr->disable_count = next_dc;

            /* Try to acquire. */
            const u32 v = LoadAcquireExclusiveForBusyMutex(p);

            /* We can only acquire read lock if not write-locked. */
            const bool write_locked = InternalReaderWriterBusyMutexValue::IsWriteLocked(v);
            if (AMS_LIKELY(!write_locked)) {
                /* Check that we don't overflow the reader count. */
                const u32 new_v = v + 1;
                AMS_ABORT_UNLESS(InternalReaderWriterBusyMutexValue::GetReaderCount(new_v) != 0);

                /* Try to store our updated lock value. */
                if (AMS_LIKELY(StoreExclusiveForBusyMutex(p, new_v))) {
                    break;
                }
            }

            /* Reset the disable counter, since we failed to acquire. */
            tlr->disable_count = cur_dc;

            /* If we don't hold any other busy mutexes, acknowledge any interrupts that occurred while we tried to acquire the lock. */
            if (cur_dc == 0 && tlr->interrupt_flag) {
                svc::SynchronizePreemptionState();
            }

            /* If the lock is held by another core, wait for it to be released. */
            if (write_locked) {
                WaitForEventsForBusyMutex();
            }
        }
    }

    void InternalReaderWriterBusyMutexImpl::ReleaseReadLock() {
        /* Release the read lock. */
        {
            /* Get pointer to our value. */
            u32 * const p = std::addressof(m_value);

            u32 v;
            do {
                /* Get and validate the current value. */
                v = LoadExclusiveForBusyMutex(p);
                AMS_ABORT_UNLESS(InternalReaderWriterBusyMutexValue::GetReaderCount(v) != 0);
            } while (!StoreReleaseExclusiveForBusyMutex(p, v - 1));
        }

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

    void InternalReaderWriterBusyMutexImpl::AcquireWriteLock() {
        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Determine disable counters. */
        const auto cur_dc = tlr->disable_count;
        AMS_ABORT_UNLESS(cur_dc < std::numeric_limits<decltype(cur_dc)>::max());
        const auto next_dc = cur_dc + 1;

        /* Check that we're allowed to use busy mutexes. */
        CallCheckBusyMutexPermission();

        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Pre-fetch the busy mutex. */
        PrefetchForBusyMutex(p);

        /* Acquire the read-lock for the mutex. */
        while (true) {
            /* Set the updated disable counter. */
            tlr->disable_count = next_dc;

            /* Try to acquire. */
            const u32 v = LoadAcquireExclusiveForBusyMutex(p);

            /* Check that we can write lock. */
            AMS_ABORT_UNLESS(static_cast<u8>(InternalReaderWriterBusyMutexValue::GetWriterNext(v) - InternalReaderWriterBusyMutexValue::GetWriterCurrent(v)) < InternalReaderWriterBusyMutexValue::WriterCountMax);

            /* Determine our write-lock number. */
            const u32 new_v = InternalReaderWriterBusyMutexValue::IncrementWriterNext(v);

            /* Try to store our updated lock value. */
            if (AMS_UNLIKELY(!StoreExclusiveForBusyMutex(p, new_v))) {
                /* Reset the disable counter, since we failed to acquire. */
                tlr->disable_count = cur_dc;

                /* If we don't hold any other busy mutexes, acknowledge any interrupts that occurred while we tried to acquire the lock. */
                if (cur_dc == 0 && tlr->interrupt_flag) {
                    svc::SynchronizePreemptionState();
                }

                continue;
            }

            /* Wait until the lock is truly acquired. */
            if (InternalReaderWriterBusyMutexValue::GetReaderCount(new_v) != 0 || InternalReaderWriterBusyMutexValue::GetWriterNext(v) != InternalReaderWriterBusyMutexValue::GetWriterCurrent(new_v)) {
                /* Send an event, so that we can immediately wait without fail. */
                SendEventLocalForBusyMutex();

                while (true) {
                    /* Wait for a lock update. */
                    WaitForEventsForBusyMutex();

                    /* Get the updated value. */
                    const u32 cur_v = LoadAcquireExclusiveForBusyMutex(p);
                    if (InternalReaderWriterBusyMutexValue::GetReaderCount(cur_v) == 0 && InternalReaderWriterBusyMutexValue::GetWriterNext(v) == InternalReaderWriterBusyMutexValue::GetWriterCurrent(cur_v)) {
                        break;
                    }
                }
            }

            /* We've acquired the write lock. */
            break;
        }
    }

    void InternalReaderWriterBusyMutexImpl::ReleaseWriteLock() {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(InternalReaderWriterBusyMutexValue::IsWriteLocked(m_value));

        /* Get pointer to our value. */
        u32 * const p = std::addressof(m_value);

        /* Release the write lock. */
        StoreReleaseWriteLockValueForBusyMutex(p);

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
