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
#include <stratosphere/os/impl/os_internal_rw_busy_mutex_value.hpp>

namespace ams::os::impl {

    /* NOTE: The current implementation is based on load/linked store conditional, we should figure out how to do this on x64. */

    class InternalReaderWriterBusyMutexImpl {
        private:
            u32 m_value;
        public:
            constexpr InternalReaderWriterBusyMutexImpl() : m_value(0) { if (!std::is_constant_evaluated()) { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); } }

            constexpr void Initialize() { m_value = 0; if (!std::is_constant_evaluated()) { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); } }

            void AcquireReadLock() { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); }
            void ReleaseReadLock() { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); }

            void AcquireWriteLock() { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); }
            void ReleaseWriteLock() { AMS_ABORT("TODO: macOS InternalReaderWriterBusyMutexImpl"); }
    };

    #define AMS_OS_INTERNAL_READER_WRITER_BUSY_MUTEX_IMPL_CONSTANT_INITIALIZER {0}

}
