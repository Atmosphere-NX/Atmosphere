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
#include "impl/os_thread_manager.hpp"
#include "impl/os_mutex_impl.hpp"

namespace ams::os {

    namespace impl {

        #ifdef ATMOSPHERE_BUILD_FOR_AUDITING

            void PushAndCheckLockLevel(const MutexType *mutex) {
                /* If auditing isn't specified, don't bother. */
                if (mutex->lock_level == 0) {
                    return;
                }

                /* TODO: Implement mutex level auditing. */
            }

            void PopAndCheckLockLevel(const MutexType *mutex) {
                /* If auditing isn't specified, don't bother. */
                if (mutex->lock_level == 0) {
                    return;
                }

                /* TODO: Implement mutex level auditing. */
            }

        #else

            void PushAndCheckLockLevel(const MutexType *mutex) {
                AMS_UNUSED(mutex);
            }

            void PopAndCheckLockLevel(const MutexType *mutex) {
                AMS_UNUSED(mutex);
            }

        #endif

    }

    namespace {

        ALWAYS_INLINE void AfterLockMutex(MutexType *mutex, ThreadType *cur_thread) {
            AMS_ASSERT(mutex->nest_count < MutexRecursiveLockCountMax);

            impl::PushAndCheckLockLevel(mutex);

            ++mutex->nest_count;
            mutex->owner_thread = cur_thread;
        }

    }

    void InitializeMutex(MutexType *mutex, bool recursive, int lock_level) {
        AMS_ASSERT((lock_level == 0) || (MutexLockLevelMin <= lock_level && lock_level <= MutexLockLevelMax));

        /* Create object. */
        util::ConstructAt(mutex->_storage);

        /* Set member variables. */
        mutex->is_recursive = recursive;
        mutex->lock_level   = lock_level;
        mutex->nest_count   = 0;
        mutex->owner_thread = nullptr;

        /* Mark initialized. */
        mutex->state = MutexType::State_Initialized;
    }

    void FinalizeMutex(MutexType *mutex) {
        AMS_ASSERT(mutex->state == MutexType::State_Initialized);

        /* Mark not intialized. */
        mutex->state = MutexType::State_NotInitialized;

        /* Destroy object. */
        util::DestroyAt(mutex->_storage);
    }

    void LockMutex(MutexType *mutex) {
        AMS_ASSERT(mutex->state == MutexType::State_Initialized);

        ThreadType *current = impl::GetCurrentThread();

        if (!mutex->is_recursive) {
            AMS_ASSERT(mutex->owner_thread != current);
            GetReference(mutex->_storage).Enter();
        } else {
            if (mutex->owner_thread == current) {
                AMS_ASSERT(mutex->nest_count >= 1);
            } else {
                GetReference(mutex->_storage).Enter();
            }
        }

        AfterLockMutex(mutex, current);
    }

    bool TryLockMutex(MutexType *mutex) {
        AMS_ASSERT(mutex->state == MutexType::State_Initialized);

        ThreadType *current = impl::GetCurrentThread();

        if (!mutex->is_recursive) {
            AMS_ASSERT(mutex->owner_thread != current);
            if (!GetReference(mutex->_storage).TryEnter()) {
                return false;
            }
        } else {
            if (mutex->owner_thread == current) {
                AMS_ASSERT(mutex->nest_count >= 1);
            } else {
                if (!GetReference(mutex->_storage).TryEnter()) {
                    return false;
                }
            }
        }

        AfterLockMutex(mutex, current);

        return true;
    }

    void UnlockMutex(MutexType *mutex) {
        AMS_ASSERT(mutex->state == MutexType::State_Initialized);
        AMS_ASSERT(mutex->nest_count > 0);
        AMS_ASSERT(mutex->owner_thread == impl::GetCurrentThread());

        impl::PopAndCheckLockLevel(mutex);

        if ((--mutex->nest_count) == 0) {
            mutex->owner_thread = nullptr;
            GetReference(mutex->_storage).Leave();
        }
    }

    bool IsMutexLockedByCurrentThread(const MutexType *mutex) {
        AMS_ASSERT(mutex->state == MutexType::State_Initialized);

        return mutex->owner_thread == impl::GetCurrentThread();
    }

}
