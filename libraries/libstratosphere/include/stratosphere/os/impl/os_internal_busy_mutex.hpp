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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/os/impl/os_internal_busy_mutex_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalBusyMutexImpl"
#endif

namespace ams::os::impl {

    class InternalBusyMutex {
        private:
            InternalBusyMutexImpl m_impl;
        public:
            constexpr InternalBusyMutex() : m_impl() { /* ... */ }

            constexpr void Initialize() { m_impl.Initialize(); }
            constexpr void Finalize()   { m_impl.Finalize(); }

            bool IsLocked() const { return m_impl.IsLocked(); }

            ALWAYS_INLINE void Lock()    { return m_impl.Lock(); }
            ALWAYS_INLINE bool TryLock() { return m_impl.TryLock(); }
            ALWAYS_INLINE void Unlock()  { return m_impl.Unlock(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }
    };

    using InternalBusyMutexStorage = util::TypedStorage<InternalBusyMutex>;

}
