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
#include <stratosphere.hpp>
#include <chrono>

namespace ams::os::impl {

    class TickManagerImpl {
        private:
            using StandardClock = typename std::conditional<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>::type;
            using TimePoint     = std::chrono::time_point<StandardClock>;
        private:
            TimePoint m_start_time;
        public:
            TickManagerImpl() : m_start_time(StandardClock::now()) { /* ... */ }

            ALWAYS_INLINE Tick GetTick() const {
                return Tick(static_cast<s64>((StandardClock::now() - m_start_time).count()));
            }

            ALWAYS_INLINE Tick GetSystemTickOrdered() const {
                PerformOrderingForGetSystemTickOrdered(true);
                ON_SCOPE_EXIT { PerformOrderingForGetSystemTickOrdered(false); };

                return Tick(static_cast<s64>((StandardClock::now() - m_start_time).count()));
            }

            static constexpr ALWAYS_INLINE s64 GetTickFrequency() {
                return static_cast<s64>(StandardClock::period::den) / static_cast<s64>(StandardClock::period::num);
            }

            static constexpr ALWAYS_INLINE s64 GetMaxTick() {
                static_assert(GetTickFrequency() <= TimeSpan::FromSeconds(1).GetNanoSeconds());
                return (std::numeric_limits<s64>::max() / TimeSpan::FromSeconds(1).GetNanoSeconds()) * GetTickFrequency();
            }

            static constexpr ALWAYS_INLINE s64 GetMaxTimeSpanNs() {
                static_assert(GetTickFrequency() <= TimeSpan::FromSeconds(1).GetNanoSeconds());
                return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()).GetNanoSeconds();
            }
        private:
            static ALWAYS_INLINE void PerformOrderingForGetSystemTickOrdered(bool before) {
                #if defined(ATMOSPHERE_ARCH_X64) || defined(ATMOSPHERE_ARCH_X86)
                int a = 0, b, c = 0, d;
                __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(a), "2"(c) : "memory");
                AMS_UNUSED(before);
                #elif defined(ATMOSPHERE_ARCH_ARM64)
                if (before) {
                    __asm__ __volatile__("dsb ish" ::: "memory");
                }
                __asm__ __volatile__("isb" ::: "memory");
                #else
                    #error "Unknown architecture for std::chrono TickManager ordering."
                #endif
            }
    };

}
