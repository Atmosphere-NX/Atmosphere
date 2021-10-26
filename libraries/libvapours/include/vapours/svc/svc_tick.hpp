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
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_select_hardware_constants.hpp>

namespace ams::svc {

    class Tick {
        public:
            static constexpr s64 TicksPerSecond = ::ams::svc::TicksPerSecond;
            static consteval s64 GetTicksPerSecond() { return TicksPerSecond; }
        private:
            s64 m_tick;
        private:
            static constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();

            static constexpr ALWAYS_INLINE s64 ConvertTimeSpanToTickImpl(TimeSpan ts) {
                /* Get nano-seconds. */
                const s64 ns = ts.GetNanoSeconds();

                /* Special-case optimize arm64/nintendo-nx value. */
                if (!std::is_constant_evaluated()) {
                    if constexpr (TicksPerSecond == 19'200'000) {
                        #if defined(ATMOSPHERE_IS_MESOSPHERE) && defined(ATMOSPHERE_ARCH_ARM64)
                        s64 t0, t1, t2, t3;
                        __asm__ __volatile__("mov   %[t1], #0x5A53\n"
                                             "movk  %[t1], #0xA09B, lsl #16\n"
                                             "lsr   %[t0], %[ns], #9\n"
                                             "movk  %[t1], #0xB82F, lsl #32\n"
                                             "movk  %[t1], #0x0044, lsl #48\n"
                                             "umulh %[t0], %[t0], %[t1]\n"
                                             "mov   %[t1], #0xFFFFFFFFFFFF3600\n"
                                             "movk  %[t1], #0xC465, lsl #16\n"
                                             "lsr   %[t0], %[t0], #0xB\n"
                                             "madd  %[t1], %[t0], %[t1], %[ns]\n"
                                             "mov   %w[t2], #0xF800\n"
                                             "movk  %w[t2], #0x0124, lsl #16\n"
                                             "mov   %w[t3], #0xCA00\n"
                                             "movk  %w[t3], #0x3B9A, lsl #16\n"
                                             "madd  %[t1], %[t1], %[t2], %[t3]\n"
                                             "mov   %[t3], #0x94B3\n"
                                             "movk  %[t3], #0x26D6, lsl #16\n"
                                             "movk  %[t3], #0x0BE8, lsl #32\n"
                                             "movk  %[t3], #0x112E, lsl #48\n"
                                             "sub   %[t1], %[t1], #1\n"
                                             "smulh %[t1], %[t1], %[t3]\n"
                                             "asr   %[t3], %[t1], #26\n"
                                             "add   %[t1], %[t3], %[t1], lsr #63\n"
                                             "madd  %[t0], %[t0], %[t2], %[t1]\n"
                                             : [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3)
                                             : [ns]"r"(ns)
                                             : "cc");
                        return t0;
                        #endif
                    }
                }

                return util::ScaleByConstantFactorUp<s64, TicksPerSecond, NanoSecondsPerSecond>(ns);
            }
        public:
            constexpr ALWAYS_INLINE explicit Tick(s64 t = 0) : m_tick(t) { /* ... */ }
            constexpr ALWAYS_INLINE Tick(TimeSpan ts) : m_tick(ConvertTimeSpanToTickImpl(ts)) { /* ... */ }

            constexpr ALWAYS_INLINE operator s64() const { return m_tick; }

            /* Tick arithmetic. */
            constexpr ALWAYS_INLINE Tick &operator+=(Tick rhs) { m_tick += rhs.m_tick; return *this; }
            constexpr ALWAYS_INLINE Tick &operator-=(Tick rhs) { m_tick -= rhs.m_tick; return *this; }
            constexpr ALWAYS_INLINE Tick operator+(Tick rhs) const { Tick r(*this); return r += rhs; }
            constexpr ALWAYS_INLINE Tick operator-(Tick rhs) const { Tick r(*this); return r -= rhs; }

            constexpr ALWAYS_INLINE Tick &operator+=(TimeSpan rhs) { m_tick += Tick(rhs).m_tick; return *this; }
            constexpr ALWAYS_INLINE Tick &operator-=(TimeSpan rhs) { m_tick -= Tick(rhs).m_tick; return *this; }
            constexpr ALWAYS_INLINE Tick operator+(TimeSpan rhs) const { Tick r(*this); return r += rhs; }
            constexpr ALWAYS_INLINE Tick operator-(TimeSpan rhs) const { Tick r(*this); return r -= rhs; }
    };

}
