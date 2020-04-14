/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
    #error "Unknown OS for os::ReadWriteLockTargetImpl"
#endif

namespace ams::os::impl {

    static_assert(alignof(ReadWriteLockType) == sizeof(u64) || alignof(ReadWriteLockType) == sizeof(u32));
    constexpr inline bool IsReadWriteLockGuaranteedAlignment = alignof(ReadWriteLockType) == sizeof(u64);

    struct ReadWriteLockCounter {
        using ReadLockCount        = util::BitPack32::Field<                        0, BITSIZEOF(u16) - 1, u32>;
        using WriteLocked          = util::BitPack32::Field<      ReadLockCount::Next,                  1, u32>;
        using ReadLockWaiterCount  = util::BitPack32::Field<        WriteLocked::Next,      BITSIZEOF(u8), u32>;
        using WriteLockWaiterCount = util::BitPack32::Field<ReadLockWaiterCount::Next,      BITSIZEOF(u8), u32>;
    };
    static_assert(ReadWriteLockCounter::WriteLockWaiterCount::Next == BITSIZEOF(u32));

    ALWAYS_INLINE void ClearReadLockCount(ReadWriteLockType::LockCount &lc) {
        lc.counter.Set<ReadWriteLockCounter::ReadLockCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLocked(ReadWriteLockType::LockCount &lc) {
        lc.counter.Set<ReadWriteLockCounter::WriteLocked>(0);
    }

    ALWAYS_INLINE void ClearReadLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        lc.counter.Set<ReadWriteLockCounter::ReadLockWaiterCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        lc.counter.Set<ReadWriteLockCounter::WriteLockWaiterCount>(0);
    }

    ALWAYS_INLINE void ClearWriteLockCount(ReadWriteLockType *rw_lock) {
        if constexpr (IsReadWriteLockGuaranteedAlignment) {
            rw_lock->lock_count.aligned.write_lock_count = 0;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                rw_lock->lock_count.not_aligned.write_lock_count = 0;
            } else {
                rw_lock->lock_count.aligned.write_lock_count = 0;
            }
        }
    }

    ALWAYS_INLINE ReadWriteLockType::LockCount &GetLockCount(ReadWriteLockType *rw_lock) {
        if constexpr (IsReadWriteLockGuaranteedAlignment) {
            return rw_lock->lock_count.aligned.c;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                return rw_lock->lock_count.not_aligned.c;
            } else {
                return rw_lock->lock_count.aligned.c;
            }
        }
    }

    ALWAYS_INLINE const ReadWriteLockType::LockCount &GetLockCount(const ReadWriteLockType *rw_lock) {
        if constexpr (IsReadWriteLockGuaranteedAlignment) {
            return rw_lock->lock_count.aligned.c;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock->lock_count)) & sizeof(u32)) {
                return rw_lock->lock_count.not_aligned.c;
            } else {
                return rw_lock->lock_count.aligned.c;
            }
        }
    }

    ALWAYS_INLINE u32 GetReadLockCount(const ReadWriteLockType::LockCount &lc) {
        return lc.counter.Get<ReadWriteLockCounter::ReadLockCount>();
    }

    ALWAYS_INLINE u32 GetWriteLocked(const ReadWriteLockType::LockCount &lc) {
        return lc.counter.Get<ReadWriteLockCounter::WriteLocked>();
    }

    ALWAYS_INLINE u32 GetReadLockWaiterCount(const ReadWriteLockType::LockCount &lc) {
        return lc.counter.Get<ReadWriteLockCounter::ReadLockWaiterCount>();
    }

    ALWAYS_INLINE u32 GetWriteLockWaiterCount(const ReadWriteLockType::LockCount &lc) {
        return lc.counter.Get<ReadWriteLockCounter::WriteLockWaiterCount>();
    }

    ALWAYS_INLINE u32 &GetWriteLockCount(ReadWriteLockType &rw_lock) {
        if constexpr (IsReadWriteLockGuaranteedAlignment) {
            return rw_lock.lock_count.aligned.write_lock_count;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock.lock_count)) & sizeof(u32)) {
                return rw_lock.lock_count.not_aligned.write_lock_count;
            } else {
                return rw_lock.lock_count.aligned.write_lock_count;
            }
        }
    }

    ALWAYS_INLINE const u32 &GetWriteLockCount(const ReadWriteLockType &rw_lock) {
        if constexpr (IsReadWriteLockGuaranteedAlignment) {
            return rw_lock.lock_count.aligned.write_lock_count;
        } else {
            if (reinterpret_cast<uintptr_t>(std::addressof(rw_lock.lock_count)) & sizeof(u32)) {
                return rw_lock.lock_count.not_aligned.write_lock_count;
            } else {
                return rw_lock.lock_count.aligned.write_lock_count;
            }
        }
    }

    ALWAYS_INLINE void IncReadLockCount(ReadWriteLockType::LockCount &lc) {
        const u32 read_lock_count = lc.counter.Get<ReadWriteLockCounter::ReadLockCount>();
        AMS_ASSERT(read_lock_count < ReadWriteLockCountMax);
        lc.counter.Set<ReadWriteLockCounter::ReadLockCount>(read_lock_count + 1);
    }

    ALWAYS_INLINE void DecReadLockCount(ReadWriteLockType::LockCount &lc) {
        const u32 read_lock_count = lc.counter.Get<ReadWriteLockCounter::ReadLockCount>();
        AMS_ASSERT(read_lock_count > 0);
        lc.counter.Set<ReadWriteLockCounter::ReadLockCount>(read_lock_count - 1);
    }

    ALWAYS_INLINE void IncReadLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        const u32 read_lock_waiter_count = lc.counter.Get<ReadWriteLockCounter::ReadLockWaiterCount>();
        AMS_ASSERT(read_lock_waiter_count < ReadWriteLockWaiterCountMax);
        lc.counter.Set<ReadWriteLockCounter::ReadLockWaiterCount>(read_lock_waiter_count + 1);
    }

    ALWAYS_INLINE void DecReadLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        const u32 read_lock_waiter_count = lc.counter.Get<ReadWriteLockCounter::ReadLockWaiterCount>();
        AMS_ASSERT(read_lock_waiter_count > 0);
        lc.counter.Set<ReadWriteLockCounter::ReadLockWaiterCount>(read_lock_waiter_count - 1);
    }

    ALWAYS_INLINE void IncWriteLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        const u32 write_lock_waiter_count = lc.counter.Get<ReadWriteLockCounter::WriteLockWaiterCount>();
        AMS_ASSERT(write_lock_waiter_count < ReadWriteLockWaiterCountMax);
        lc.counter.Set<ReadWriteLockCounter::WriteLockWaiterCount>(write_lock_waiter_count + 1);
    }

    ALWAYS_INLINE void DecWriteLockWaiterCount(ReadWriteLockType::LockCount &lc) {
        const u32 write_lock_waiter_count = lc.counter.Get<ReadWriteLockCounter::WriteLockWaiterCount>();
        AMS_ASSERT(write_lock_waiter_count > 0);
        lc.counter.Set<ReadWriteLockCounter::WriteLockWaiterCount>(write_lock_waiter_count - 1);
    }


    ALWAYS_INLINE void IncWriteLockCount(ReadWriteLockType &rw_lock) {
        u32 &write_lock_count = GetWriteLockCount(rw_lock);
        AMS_ASSERT(write_lock_count < ReadWriteLockCountMax);
        ++write_lock_count;
    }

    ALWAYS_INLINE void DecWriteLockCount(ReadWriteLockType &rw_lock) {
        u32 &write_lock_count = GetWriteLockCount(rw_lock);
        AMS_ASSERT(write_lock_count > 0);
        --write_lock_count;
    }

    ALWAYS_INLINE void SetWriteLocked(ReadWriteLockType::LockCount &lc) {
        lc.counter.Set<ReadWriteLockCounter::WriteLocked>(1);
    }

    using ReadWriteLockImpl = ReadWriteLockTargetImpl;

}
