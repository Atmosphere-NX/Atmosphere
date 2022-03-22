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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_rw_lock_target_impl.os.horizon.hpp"
#else
    #error "Unknown OS for os::ReaderWriterLockTargetImpl"
#endif

namespace ams::os::impl {

    static_assert(alignof(ReaderWriterLockType) == sizeof(u64) || alignof(ReaderWriterLockType) == sizeof(u32));
    constexpr inline bool IsReaderWriterLockGuaranteedAlignment = alignof(ReaderWriterLockType) == sizeof(u64);

    struct ReaderWriterLockCounter {
        using ReadLockCount        = util::BitPack32::Field<                        0, BITSIZEOF(u16) - 1, u32>;
        using WriteLocked          = util::BitPack32::Field<      ReadLockCount::Next,                  1, u32>;
        using ReadLockWaiterCount  = util::BitPack32::Field<        WriteLocked::Next,      BITSIZEOF(u8), u32>;
        using WriteLockWaiterCount = util::BitPack32::Field<ReadLockWaiterCount::Next,      BITSIZEOF(u8), u32>;
    };
    static_assert(ReaderWriterLockCounter::WriteLockWaiterCount::Next == BITSIZEOF(u32));

    ALWAYS_INLINE void ClearReadLockCount(ReaderWriterLockType::LockCount &lc) {
        lc.counter.Set<ReaderWriterLockCounter::ReadLockCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLocked(ReaderWriterLockType::LockCount &lc) {
        lc.counter.Set<ReaderWriterLockCounter::WriteLocked>(0);
    }

    ALWAYS_INLINE void ClearReadLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        lc.counter.Set<ReaderWriterLockCounter::ReadLockWaiterCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        lc.counter.Set<ReaderWriterLockCounter::WriteLockWaiterCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLockCount(ReaderWriterLockType *rw_lock) {
        if constexpr (IsReaderWriterLockGuaranteedAlignment) {
            rw_lock->lock_count.aligned.write_lock_count = 0;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                rw_lock->lock_count.not_aligned.write_lock_count = 0;
            } else {
                rw_lock->lock_count.aligned.write_lock_count = 0;
            }
        }
    }

    ALWAYS_INLINE ReaderWriterLockType::LockCount &GetLockCount(ReaderWriterLockType *rw_lock) {
        if constexpr (IsReaderWriterLockGuaranteedAlignment) {
            return rw_lock->lock_count.aligned.c;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                return rw_lock->lock_count.not_aligned.c;
            } else {
                return rw_lock->lock_count.aligned.c;
            }
        }
    }

    ALWAYS_INLINE const ReaderWriterLockType::LockCount &GetLockCount(const ReaderWriterLockType *rw_lock) {
        if constexpr (IsReaderWriterLockGuaranteedAlignment) {
            return rw_lock->lock_count.aligned.c;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                return rw_lock->lock_count.not_aligned.c;
            } else {
                return rw_lock->lock_count.aligned.c;
            }
        }
    }

    ALWAYS_INLINE u32 GetReadLockCount(const ReaderWriterLockType::LockCount &lc) {
        return lc.counter.Get<ReaderWriterLockCounter::ReadLockCount>();
    }

    ALWAYS_INLINE u32 GetWriteLocked(const ReaderWriterLockType::LockCount &lc) {
        return lc.counter.Get<ReaderWriterLockCounter::WriteLocked>();
    }

    ALWAYS_INLINE u32 GetReadLockWaiterCount(const ReaderWriterLockType::LockCount &lc) {
        return lc.counter.Get<ReaderWriterLockCounter::ReadLockWaiterCount>();
    }

    ALWAYS_INLINE u32 GetWriteLockWaiterCount(const ReaderWriterLockType::LockCount &lc) {
        return lc.counter.Get<ReaderWriterLockCounter::WriteLockWaiterCount>();
    }

    ALWAYS_INLINE u32 &GetWriteLockCount(ReaderWriterLockType &rw_lock) {
        if constexpr (IsReaderWriterLockGuaranteedAlignment) {
            return rw_lock.lock_count.aligned.write_lock_count;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock.lock_count)) & sizeof(u32)) {
                return rw_lock.lock_count.not_aligned.write_lock_count;
            } else {
                return rw_lock.lock_count.aligned.write_lock_count;
            }
        }
    }

    ALWAYS_INLINE const u32 &GetWriteLockCount(const ReaderWriterLockType &rw_lock) {
        if constexpr (IsReaderWriterLockGuaranteedAlignment) {
            return rw_lock.lock_count.aligned.write_lock_count;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock.lock_count)) & sizeof(u32)) {
                return rw_lock.lock_count.not_aligned.write_lock_count;
            } else {
                return rw_lock.lock_count.aligned.write_lock_count;
            }
        }
    }

    ALWAYS_INLINE void IncReadLockCount(ReaderWriterLockType::LockCount &lc) {
        const u32 read_lock_count = lc.counter.Get<ReaderWriterLockCounter::ReadLockCount>();
        AMS_ASSERT(read_lock_count < ReaderWriterLockCountMax);
        lc.counter.Set<ReaderWriterLockCounter::ReadLockCount>(read_lock_count + 1);
    }

    ALWAYS_INLINE void DecReadLockCount(ReaderWriterLockType::LockCount &lc) {
        const u32 read_lock_count = lc.counter.Get<ReaderWriterLockCounter::ReadLockCount>();
        AMS_ASSERT(read_lock_count > 0);
        lc.counter.Set<ReaderWriterLockCounter::ReadLockCount>(read_lock_count - 1);
    }

    ALWAYS_INLINE void IncReadLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        const u32 read_lock_waiter_count = lc.counter.Get<ReaderWriterLockCounter::ReadLockWaiterCount>();
        AMS_ASSERT(read_lock_waiter_count < ReaderWriterLockWaiterCountMax);
        lc.counter.Set<ReaderWriterLockCounter::ReadLockWaiterCount>(read_lock_waiter_count + 1);
    }

    ALWAYS_INLINE void DecReadLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        const u32 read_lock_waiter_count = lc.counter.Get<ReaderWriterLockCounter::ReadLockWaiterCount>();
        AMS_ASSERT(read_lock_waiter_count > 0);
        lc.counter.Set<ReaderWriterLockCounter::ReadLockWaiterCount>(read_lock_waiter_count - 1);
    }

    ALWAYS_INLINE void IncWriteLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        const u32 write_lock_waiter_count = lc.counter.Get<ReaderWriterLockCounter::WriteLockWaiterCount>();
        AMS_ASSERT(write_lock_waiter_count < ReaderWriterLockWaiterCountMax);
        lc.counter.Set<ReaderWriterLockCounter::WriteLockWaiterCount>(write_lock_waiter_count + 1);
    }

    ALWAYS_INLINE void DecWriteLockWaiterCount(ReaderWriterLockType::LockCount &lc) {
        const u32 write_lock_waiter_count = lc.counter.Get<ReaderWriterLockCounter::WriteLockWaiterCount>();
        AMS_ASSERT(write_lock_waiter_count > 0);
        lc.counter.Set<ReaderWriterLockCounter::WriteLockWaiterCount>(write_lock_waiter_count - 1);
    }


    ALWAYS_INLINE void IncWriteLockCount(ReaderWriterLockType &rw_lock) {
        u32 &write_lock_count = GetWriteLockCount(rw_lock);
        AMS_ASSERT(write_lock_count < ReaderWriterLockCountMax);
        ++write_lock_count;
    }

    ALWAYS_INLINE void DecWriteLockCount(ReaderWriterLockType &rw_lock) {
        u32 &write_lock_count = GetWriteLockCount(rw_lock);
        AMS_ASSERT(write_lock_count > 0);
        --write_lock_count;
    }

    ALWAYS_INLINE void SetWriteLocked(ReaderWriterLockType::LockCount &lc) {
        lc.counter.Set<ReaderWriterLockCounter::WriteLocked>(1);
    }

    using ReaderWriterLockImpl = ReaderWriterLockTargetImpl;

}
