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
    #include <stratosphere/os/impl/os_internal_critical_section_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalCriticalSectionImpl"
#endif

namespace ams::os::impl {

    class InternalCriticalSection {
        private:
            InternalCriticalSectionImpl m_impl;
        public:
            constexpr InternalCriticalSection() : m_impl() { /* ... */ }

            constexpr void Initialize() { m_impl.Initialize(); }
            constexpr void Finalize()   { m_impl.Finalize(); }

            void Enter()    { return m_impl.Enter(); }
            bool TryEnter() { return m_impl.TryEnter(); }
            void Leave()    { return m_impl.Leave(); }

            bool IsLockedByCurrentThread() const { return m_impl.IsLockedByCurrentThread(); }

            ALWAYS_INLINE void Lock()    { return this->Enter(); }
            ALWAYS_INLINE bool TryLock() { return this->TryEnter(); }
            ALWAYS_INLINE void Unlock()  { return this->Leave(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }

            InternalCriticalSectionImpl *Get() {
                return std::addressof(m_impl);
            }

            const InternalCriticalSectionImpl *Get() const {
                return std::addressof(m_impl);
            }
    };

    using InternalCriticalSectionStorage = util::TypedStorage<InternalCriticalSection>;

}
