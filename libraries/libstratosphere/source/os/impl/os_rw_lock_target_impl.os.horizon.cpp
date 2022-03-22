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
#include <stratosphere.hpp>
#include "os_rw_lock_impl.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    #define ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(dst_ref, expected_ref, desired_ref, success, fail) \
        (__atomic_compare_exchange(reinterpret_cast<u64 *>(std::addressof(dst_ref)), reinterpret_cast<u64 *>(std::addressof(expected_ref)), reinterpret_cast<u64 *>(std::addressof(desired_ref)), true, success, fail))


    void ReaderWriterLockHorizonImpl::AcquireReadLockWriteLocked(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
        AMS_ASSERT((GetThreadHandle(GetLockCount(rw_lock)) | svc::HandleWaitMask) == (impl::GetCurrentThreadHandle() | svc::HandleWaitMask));
        alignas(alignof(u64)) LockCount expected   = GetLockCount(rw_lock);
        alignas(alignof(u64)) LockCount lock_count;
        do {
            lock_count = expected;
            IncReadLockCount(lock_count);
        } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
    }

    void ReaderWriterLockHorizonImpl::ReleaseReadLockWriteLocked(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
        AMS_ASSERT((GetThreadHandle(GetLockCount(rw_lock)) | svc::HandleWaitMask) == (impl::GetCurrentThreadHandle() | svc::HandleWaitMask));
        alignas(alignof(u64)) LockCount expected   = GetLockCount(rw_lock);
        alignas(alignof(u64)) LockCount lock_count;
        do {
            lock_count = expected;
            DecReadLockCount(lock_count);
        } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
        return ReleaseWriteLockImpl(rw_lock);
    }

    void ReaderWriterLockHorizonImpl::ReleaseWriteLockImpl(os::ReaderWriterLockType *rw_lock) {
        alignas(alignof(u64)) LockCount expected   = GetLockCount(rw_lock);
        alignas(alignof(u64)) LockCount lock_count = expected;
        AMS_ASSERT(GetWriteLocked(lock_count) == 1);
        if (GetReadLockCount(lock_count) == 0 && GetWriteLockCount(*rw_lock) == 0) {
            rw_lock->owner_thread = nullptr;

            if (GetWriteLockWaiterCount(lock_count) > 0) {
                GetReference(rw_lock->cv_write_lock._storage).Signal();
            } else if (GetReadLockWaiterCount(lock_count) > 0) {
                GetReference(rw_lock->cv_read_lock._storage).Broadcast();
            }

            do {
                ClearWriteLocked(lock_count);
                SetThreadHandle(lock_count, (GetThreadHandle(lock_count) & svc::HandleWaitMask) ? GetThreadHandle(lock_count) : svc::InvalidHandle);
            } while(!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELEASE, __ATOMIC_RELAXED));

            if (GetThreadHandle(lock_count)) {
                R_ABORT_UNLESS(svc::ArbitrateUnlock(GetThreadHandleAddress(GetLockCount(rw_lock))));
            }
        }
    }

    void ReaderWriterLockHorizonImpl::AcquireReadLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        auto *cur_thread = impl::GetCurrentThread();
        if (rw_lock->owner_thread == cur_thread) {
            return AcquireReadLockWriteLocked(rw_lock);
        } else {
            const u32 cur_handle = cur_thread->thread_impl->handle;
            bool arbitrated = false;
            bool got_read_lock, needs_arbitrate_lock;

            alignas(alignof(u64)) LockCount lock_count;
            alignas(alignof(u64)) LockCount expected;
            while (true) {
                expected = GetLockCount(rw_lock);
                do {
                    lock_count = expected;
                    AMS_ASSERT(GetWriteLocked(lock_count) == 0 || GetThreadHandle(lock_count) != svc::InvalidHandle);

                    if (GetWriteLocked(lock_count) != 0 || GetWriteLockWaiterCount(lock_count) != 0) {
                        if (!arbitrated) {
                            IncReadLockWaiterCount(lock_count);
                        }
                        const u32 h = GetThreadHandle(lock_count);
                        got_read_lock = false;
                        needs_arbitrate_lock = h != 0;
                        SetThreadHandle(lock_count, needs_arbitrate_lock ? (h | svc::HandleWaitMask) : cur_handle);
                    } else {
                        if (arbitrated) {
                            DecReadLockWaiterCount(lock_count);
                        }
                        IncReadLockCount(lock_count);
                        got_read_lock = true;
                        needs_arbitrate_lock = false;
                    }
                } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

                if (!needs_arbitrate_lock) {
                    break;
                }

                R_ABORT_UNLESS(svc::ArbitrateLock(GetThreadHandle(lock_count) & ~svc::HandleWaitMask, GetThreadHandleAddress(GetLockCount(rw_lock)), cur_handle));

                const u32 new_handle = GetThreadHandle(GetLockCount(rw_lock));
                if ((new_handle | svc::HandleWaitMask) == (cur_handle | svc::HandleWaitMask)) {
                    lock_count = GetLockCount(rw_lock);
                    break;
                }

                arbitrated = true;
            }

            if (got_read_lock) {
                return;
            }

            expected = lock_count;
            do {
                while (GetWriteLockWaiterCount(lock_count)) {
                    GetReference(rw_lock->cv_read_lock._storage).Wait(GetPointer(GetLockCount(rw_lock).cs_storage));
                    expected   = GetLockCount(rw_lock);
                    lock_count = expected;
                }
                DecReadLockWaiterCount(lock_count);
                IncReadLockCount(lock_count);
                SetThreadHandle(lock_count, (GetThreadHandle(lock_count) & svc::HandleWaitMask) ? GetThreadHandle(lock_count) : svc::InvalidHandle);
            } while(!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
            if (GetThreadHandle(lock_count) & svc::HandleWaitMask) {
                R_ABORT_UNLESS(svc::ArbitrateUnlock(GetThreadHandleAddress(GetLockCount(rw_lock))));
            }
        }
    }

    bool ReaderWriterLockHorizonImpl::TryAcquireReadLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        auto *cur_thread = impl::GetCurrentThread();
        if (rw_lock->owner_thread == cur_thread) {
            AcquireReadLockWriteLocked(rw_lock);
            return true;
        } else {
            alignas(alignof(u64)) LockCount expected   = GetLockCount(rw_lock);
            alignas(alignof(u64)) LockCount lock_count;
            while (true) {
                lock_count = expected;
                AMS_ASSERT(GetWriteLocked(lock_count) == 0 || GetThreadHandle(lock_count) != 0);
                if (GetWriteLocked(lock_count) == 0 && GetWriteLockWaiterCount(lock_count) == 0) {
                    IncReadLockCount(lock_count);
                    if (ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                        return true;
                    }
                } else {
                    return false;
                }
            }
        }
    }

    void ReaderWriterLockHorizonImpl::ReleaseReadLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        AMS_ASSERT(GetReadLockCount(GetLockCount(rw_lock)) > 0);
        auto *cur_thread = impl::GetCurrentThread();
        if (rw_lock->owner_thread == cur_thread) {
            return ReleaseReadLockWriteLocked(rw_lock);
        } else {
            alignas(alignof(u64)) LockCount expected = GetLockCount(rw_lock);
            alignas(alignof(u64)) LockCount lock_count;
            while (true) {
                lock_count = expected;
                DecReadLockCount(lock_count);
                if (GetReadLockCount(lock_count) == 0 && GetWriteLockWaiterCount(lock_count) != 0) {
                    break;
                }
                if (ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
                    return;
                }
            }

            AMS_ASSERT(GetWriteLocked(lock_count) == 0);

            const u32 cur_handle = cur_thread->thread_impl->handle;
            do {
                do {
                    const u32 h = GetThreadHandle(lock_count);
                    SetThreadHandle(lock_count, h != 0 ? (h | svc::HandleWaitMask) : cur_handle);
                } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

                if (GetThreadHandle(lock_count) == cur_handle) {
                    break;
                }

                R_ABORT_UNLESS(svc::ArbitrateLock(GetThreadHandle(lock_count) & ~svc::HandleWaitMask, GetThreadHandleAddress(GetLockCount(rw_lock)), cur_handle));
                expected = GetLockCount(rw_lock);
            } while ((GetThreadHandle(GetLockCount(rw_lock)) | svc::HandleWaitMask) != (cur_handle | svc::HandleWaitMask));

            do {
                lock_count = expected;
                AMS_ASSERT(GetReadLockCount(lock_count) == 1 && GetWriteLockWaiterCount(lock_count) > 0);
                DecReadLockCount(lock_count);
            } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELAXED, __ATOMIC_RELAXED));

            GetReference(rw_lock->cv_write_lock._storage).Signal();

            do {
                lock_count = expected;
                AMS_ASSERT(GetReadLockCount(lock_count) == 0 && GetWriteLockWaiterCount(lock_count) > 0);
                SetThreadHandle(lock_count, (GetThreadHandle(lock_count) & svc::HandleWaitMask) ? GetThreadHandle(lock_count) : svc::InvalidHandle);
            } while(!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_RELEASE, __ATOMIC_RELAXED));

            if (GetThreadHandle(lock_count) & svc::HandleWaitMask) {
                R_ABORT_UNLESS(svc::ArbitrateUnlock(GetThreadHandleAddress(GetLockCount(rw_lock))));
            }
        }
    }

    void ReaderWriterLockHorizonImpl::AcquireWriteLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        auto *cur_thread = impl::GetCurrentThread();
        const u32 cur_handle = cur_thread->thread_impl->handle;
        if (rw_lock->owner_thread == cur_thread) {
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
            AMS_ASSERT((GetThreadHandle(GetLockCount(rw_lock)) | svc::HandleWaitMask) == (cur_handle | svc::HandleWaitMask));
            IncWriteLockCount(*rw_lock);
        } else {
            alignas(alignof(u64)) LockCount expected;
            alignas(alignof(u64)) LockCount lock_count;
            bool arbitrated = false;
            bool got_write_lock, needs_arbitrate_lock;

            while (true) {
                expected = GetLockCount(rw_lock);
                do {
                    lock_count = expected;
                    AMS_ASSERT(GetWriteLocked(lock_count) == 0 || GetThreadHandle(lock_count) != svc::InvalidHandle);
                    if (GetReadLockCount(lock_count) > 0 || GetThreadHandle(lock_count) != svc::InvalidHandle) {
                        if (!arbitrated) {
                            IncWriteLockWaiterCount(lock_count);
                        }
                        const u32 h = GetThreadHandle(lock_count);
                        got_write_lock = false;
                        needs_arbitrate_lock = h != 0;
                        SetThreadHandle(lock_count, needs_arbitrate_lock ? (h | svc::HandleWaitMask) : cur_handle);
                    } else {
                        if (arbitrated) {
                            DecWriteLockWaiterCount(lock_count);
                        }
                        SetWriteLocked(lock_count);
                        got_write_lock = true;
                        needs_arbitrate_lock = false;
                    }
                } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

                if (got_write_lock) {
                    break;
                }

                if (needs_arbitrate_lock) {
                    R_ABORT_UNLESS(svc::ArbitrateLock(GetThreadHandle(lock_count) & ~svc::HandleWaitMask, GetThreadHandleAddress(GetLockCount(rw_lock)), cur_handle));
                    arbitrated = true;
                    expected   = GetLockCount(rw_lock);
                    if ((GetThreadHandle(expected) | svc::HandleWaitMask) != (cur_handle | svc::HandleWaitMask)) {
                        continue;
                    }
                }

                do {
                    lock_count = expected;

                    AMS_ASSERT(GetWriteLocked(lock_count) == 0);
                    while (GetReadLockCount(lock_count) > 0) {
                        GetReference(rw_lock->cv_write_lock._storage).Wait(GetPointer(GetLockCount(rw_lock).cs_storage));
                        expected   = GetLockCount(rw_lock);
                        lock_count = expected;
                    }
                    AMS_ASSERT(GetWriteLocked(lock_count) == 0);

                    DecWriteLockWaiterCount(lock_count);
                    SetWriteLocked(lock_count);
                } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

                break;
            }

            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            IncWriteLockCount(*rw_lock);
            rw_lock->owner_thread = cur_thread;
        }
    }

    bool ReaderWriterLockHorizonImpl::TryAcquireWriteLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        auto *cur_thread = impl::GetCurrentThread();
        if (rw_lock->owner_thread == cur_thread) {
            AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) == 1);
            IncWriteLockCount(*rw_lock);
            return true;
        } else {
            const u32 cur_handle = cur_thread->thread_impl->handle;
            alignas(alignof(u64)) LockCount expected   = GetLockCount(rw_lock);
            alignas(alignof(u64)) LockCount lock_count;

            do {
                lock_count = expected;
                AMS_ASSERT(GetWriteLocked(lock_count) == 0 || GetThreadHandle(lock_count) != svc::InvalidHandle);

                if (GetReadLockCount(lock_count) > 0 || GetThreadHandle(lock_count) != svc::InvalidHandle) {
                    return false;
                }

                SetWriteLocked(lock_count);
                SetThreadHandle(lock_count, cur_handle);
            } while (!ATOMIC_COMPARE_EXCHANGE_LOCK_COUNT(GetLockCount(rw_lock), expected, lock_count, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));


            AMS_ASSERT(GetWriteLockCount(*rw_lock) == 0);
            IncWriteLockCount(*rw_lock);
            rw_lock->owner_thread = cur_thread;
            return true;
        }
    }

    void ReaderWriterLockHorizonImpl::ReleaseWriteLock(os::ReaderWriterLockType *rw_lock) {
        AMS_ASSERT(::ams::svc::GetThreadLocalRegion()->disable_count == 0);

        AMS_ASSERT(GetWriteLockCount(*rw_lock) > 0);
        AMS_ASSERT(GetWriteLocked(GetLockCount(rw_lock)) != 0);

        DecWriteLockCount(*rw_lock);
        return ReleaseWriteLockImpl(rw_lock);
    }

}
