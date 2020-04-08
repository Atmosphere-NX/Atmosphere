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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/os/impl/os_internal_critical_section_impl.os.horizon.hpp>
#else
    #error "Unknown OS for ams::os::impl::InternalCriticalSectionImpl"
#endif

namespace ams::os::impl {

    class InternalCriticalSection {
        private:
            InternalCriticalSectionImpl impl;
        public:
            constexpr InternalCriticalSection() : impl() { /* ... */ }

            constexpr void Initialize() { this->impl.Initialize(); }
            constexpr void Finalize()   { this->impl.Finalize(); }

            void Enter()    { return this->impl.Enter(); }
            bool TryEnter() { return this->impl.TryEnter(); }
            void Leave()    { return this->impl.Leave(); }

            bool IsLockedByCurrentThread() const { return this->impl.IsLockedByCurrentThread(); }

            ALWAYS_INLINE void Lock()    { return this->Enter(); }
            ALWAYS_INLINE bool TryLock() { return this->TryEnter(); }
            ALWAYS_INLINE void Unlock()  { return this->Leave(); }

            ALWAYS_INLINE void lock()     { return this->Lock(); }
            ALWAYS_INLINE bool try_lock() { return this->TryLock(); }
            ALWAYS_INLINE void unlock()   { return this->Unlock(); }

            InternalCriticalSectionImpl *Get() {
                return std::addressof(this->impl);
            }

            const InternalCriticalSectionImpl *Get() const {
                return std::addressof(this->impl);
            }
    };

    using InternalCriticalSectionStorage = TYPED_STORAGE(InternalCriticalSection);

}
