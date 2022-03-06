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

namespace ams::os::impl {


    class InternalBusyMutexImpl {
        private:
            std::atomic<bool> m_value{false};
            u8 m_padding[3]{};
        public:
            constexpr InternalBusyMutexImpl() = default;

            ALWAYS_INLINE void Initialize() { m_value.store(false, std::memory_order_relaxed); }

            ALWAYS_INLINE void Finalize() { /* ... */ }

            ALWAYS_INLINE bool IsLocked() const { return m_value.load(std::memory_order_acquire); }

            ALWAYS_INLINE bool TryLock() {
                bool expected = false;
                return m_value.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_acquire);

            }

            ALWAYS_INLINE void Lock() {
                while (!this->TryLock()) {
                    #if defined(ATMOSPHERE_ARCH_X64) || defined(ATMOSPHERE_ARCH_X86)
                        _mm_pause();
                    #elif defined(ATMOSPHERE_ARCH_ARM64) || defined(ATMOSPHERE_ARCH_ARM)
                        __asm__ __volatile__("yield" ::: "memory");
                    #else
                        #error "InternalBusyMutex requires yield intrinsics"
                    #endif
                }
            }

            ALWAYS_INLINE void Unlock() {
                m_value.store(false, std::memory_order_release);
            }
    };
    static_assert(sizeof(InternalBusyMutexImpl) == sizeof(u32));

    #define AMS_OS_INTERNAL_BUSY_MUTEX_IMPL_CONSTANT_INITIALIZE_ARRAY_VALUES 0

}
