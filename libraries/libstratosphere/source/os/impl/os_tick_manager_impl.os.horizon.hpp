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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class TickManagerImpl {
        public:
            constexpr TickManagerImpl() { /* ... */ }

            ALWAYS_INLINE Tick GetTick() const {
                s64 tick;
                #if   defined(ATMOSPHERE_ARCH_ARM64)
                    __asm__ __volatile__("mrs %[tick], cntpct_el0" : [tick]"=&r"(tick) :: "memory");
                #else
                    #error "Unknown Architecture for TickManagerImpl::GetTick"
                #endif
                return Tick(tick);
            }

            ALWAYS_INLINE Tick GetSystemTickOrdered() const {
                s64 tick;
                #if   defined(ATMOSPHERE_ARCH_ARM64)
                    __asm__ __volatile__("dsb ish\n"
                                         "isb\n"
                                         "mrs %[tick], cntpct_el0\n"
                                         "isb"
                                         : [tick]"=&r"(tick)
                                         :
                                         : "memory");
                #else
                    #error "Unknown Architecture for TickManagerImpl::GetSystemTickOrdered"
                #endif
                return Tick(tick);
            }

            static constexpr ALWAYS_INLINE s64 GetTickFrequency() {
                return static_cast<s64>(::ams::svc::TicksPerSecond);
            }

            static constexpr ALWAYS_INLINE s64 GetMaxTick() {
                static_assert(GetTickFrequency() <= TimeSpan::FromSeconds(1).GetNanoSeconds());
                return (std::numeric_limits<s64>::max() / TimeSpan::FromSeconds(1).GetNanoSeconds()) * GetTickFrequency();
            }

            static constexpr ALWAYS_INLINE s64 GetMaxTimeSpanNs() {
                static_assert(GetTickFrequency() <= TimeSpan::FromSeconds(1).GetNanoSeconds());
                return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()).GetNanoSeconds();
            }
    };

}
