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
#include <vapours.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>

namespace ams::os {

    class SdkConditionVariable;

    struct SdkRecursiveMutexType {
        union {
            s32 _arr[sizeof(impl::InternalCriticalSectionStorage) / sizeof(s32)];
            impl::InternalCriticalSectionStorage _storage;
        };
        u32 recursive_count;
    };
    static_assert(std::is_trivial<SdkRecursiveMutexType>::value);

    void InitializeSdkRecursiveMutex(SdkRecursiveMutexType *rmutex);

    void LockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex);
    bool TryLockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex);
    void UnlockSdkRecursiveMutex(SdkRecursiveMutexType *rmutex);

    bool IsSdkRecursiveMutexLockedByCurrentThread(const SdkRecursiveMutexType *rmutex);

    class SdkRecursiveMutex {
        private:
            friend class SdkConditionVariable;
        private:
            SdkRecursiveMutexType m_mutex;
        public:
            constexpr SdkRecursiveMutex() : m_mutex{{0}, 0} { /* ... */ }

            ALWAYS_INLINE void Lock()    { return os::LockSdkRecursiveMutex(std::addressof(m_mutex)); }
            ALWAYS_INLINE bool TryLock() { return os::TryLockSdkRecursiveMutex(std::addressof(m_mutex)); }
            ALWAYS_INLINE void Unlock()  { return os::UnlockSdkRecursiveMutex(std::addressof(m_mutex)); }

            ALWAYS_INLINE bool IsLockedByCurrentThread() const { return os::IsSdkRecursiveMutexLockedByCurrentThread(std::addressof(m_mutex)); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

}
