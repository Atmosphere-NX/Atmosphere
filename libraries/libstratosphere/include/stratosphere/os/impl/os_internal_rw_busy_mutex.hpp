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
    #include <stratosphere/os/impl/os_internal_rw_busy_mutex_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalReaderWriterBusyMutexImpl"
#endif

namespace ams::os::impl {

    class InternalReaderWriterBusyMutex {
        private:
            InternalReaderWriterBusyMutexImpl m_impl;
        public:
            constexpr InternalReaderWriterBusyMutex() : m_impl() { /* ... */ }

            ALWAYS_INLINE void AcquireReadLock() { return m_impl.AcquireReadLock(); }
            ALWAYS_INLINE void ReleaseReadLock() { return m_impl.ReleaseReadLock(); }

            ALWAYS_INLINE void AcquireWriteLock() { return m_impl.AcquireWriteLock(); }
            ALWAYS_INLINE void ReleaseWriteLock() { return m_impl.ReleaseWriteLock(); }
    };

    using InternalReaderWriterBusyMutexStorage = util::TypedStorage<InternalReaderWriterBusyMutex>;

}
