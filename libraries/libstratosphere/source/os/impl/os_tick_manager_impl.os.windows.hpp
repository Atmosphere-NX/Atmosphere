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

#if defined(ATMOSPHERE_OS_WINDOWS)
#include <stratosphere/windows.hpp>
#endif

#include <mmsystem.h>

namespace ams::os::impl {

    class TickManagerImpl {
        private:
            s64 m_tick_frequency;
            TimeSpan m_max_time;
            s64 m_max_tick;
        public:
            TickManagerImpl() {
                /* Get the tick frequency. */
                ::timeBeginPeriod(1);

                LARGE_INTEGER freq;
                ::QueryPerformanceFrequency(std::addressof(freq));
                m_tick_frequency = static_cast<s64>(freq.QuadPart);

                /* Set maximums. */
                constexpr s64 TickFrequencyForNanoSecondResolution = TimeSpan::FromSeconds(1).GetNanoSeconds();
                if (m_tick_frequency <= TickFrequencyForNanoSecondResolution) {
                    m_max_tick = m_tick_frequency * (std::numeric_limits<s64>::max() / TickFrequencyForNanoSecondResolution);
                    m_max_time = TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max());
                } else {
                    m_max_tick = std::numeric_limits<s64>::max();
                    m_max_time = TimeSpan::FromSeconds(std::numeric_limits<s64>::max() / m_tick_frequency);
                }
            }

            ~TickManagerImpl() {
                ::timeEndPeriod(1);
            }

            ALWAYS_INLINE Tick GetTick() const {
                LARGE_INTEGER freq;
                ::QueryPerformanceCounter(std::addressof(freq));
                return Tick(static_cast<s64>(freq.QuadPart));
            }

            ALWAYS_INLINE Tick GetSystemTickOrdered() const {
                LARGE_INTEGER freq;

                PerformOrderingForGetSystemTickOrdered();
                ::QueryPerformanceCounter(std::addressof(freq));
                PerformOrderingForGetSystemTickOrdered();

                return Tick(static_cast<s64>(freq.QuadPart));
            }

            ALWAYS_INLINE s64 GetTickFrequency() const {
                return m_tick_frequency;
            }

            ALWAYS_INLINE s64 GetMaxTick() const {
                return m_max_tick;
            }

            ALWAYS_INLINE s64 GetMaxTimeSpanNs() const {
                return m_max_time.GetNanoSeconds();
            }
        private:
            static ALWAYS_INLINE void PerformOrderingForGetSystemTickOrdered() {
                int a = 0, b, c = 0, d;
                __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(a), "2"(c) : "memory");
            }
    };

}
