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
#include <vapours.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>

namespace ams::os {

    class SdkConditionVariable;

    struct SdkMutexType {
        union {
            s32 _arr[sizeof(impl::InternalCriticalSectionStorage) / sizeof(s32)];
            impl::InternalCriticalSectionStorage _storage;
        };
    };
    static_assert(std::is_trivial<SdkMutexType>::value);

    void InitializeSdkMutex(SdkMutexType *mutex);

    void LockSdkMutex(SdkMutexType *mutex);
    bool TryLockSdkMutex(SdkMutexType *mutex);
    void UnlockSdkMutex(SdkMutexType *mutex);

    bool IsSdkMutexLockedByCurrentThread(const SdkMutexType *mutex);

    class SdkMutex {
        private:
            friend class SdkConditionVariable;
        private:
            SdkMutexType mutex;
        public:
            constexpr SdkMutex() : mutex{{0}} { /* ... */ }

            ALWAYS_INLINE void Lock()    { return os::LockSdkMutex(std::addressof(this->mutex)); }
            ALWAYS_INLINE bool TryLock() { return os::TryLockSdkMutex(std::addressof(this->mutex)); }
            ALWAYS_INLINE void Unlock()  { return os::UnlockSdkMutex(std::addressof(this->mutex)); }

            ALWAYS_INLINE bool IsLockedByCurrentThread() const { return os::IsSdkMutexLockedByCurrentThread(std::addressof(this->mutex)); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

}
