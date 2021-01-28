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
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_select_hardware_constants.hpp>

namespace ams::svc {

    class Tick {
        public:
            static constexpr s64 TicksPerSecond = ::ams::svc::TicksPerSecond;
            static constexpr s64 GetTicksPerSecond() { return TicksPerSecond; }
        private:
            s64 tick;
        private:
            static constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();

            static constexpr void DivNs(s64 &out, const s64 value) {
                out = value / NanoSecondsPerSecond;
            }

            static constexpr void DivModNs(s64 &out_div, s64 &out_mod, const s64 value) {
                out_div = value / NanoSecondsPerSecond;
                out_mod = value % NanoSecondsPerSecond;
            }

            static constexpr s64 ConvertTimeSpanToTickImpl(TimeSpan ts) {
                /* Split up timespan and ticks-per-second by ns. */
                s64 ts_div = 0, ts_mod = 0;
                s64 tick_div = 0, tick_mod = 0;
                DivModNs(ts_div, ts_mod, ts.GetNanoSeconds());
                DivModNs(tick_div, tick_mod, TicksPerSecond);

                /* Convert the timespan into a tick count. */
                s64 value = 0;
                DivNs(value, ts_mod * tick_mod + NanoSecondsPerSecond - 1);

                return (ts_div * tick_div) * NanoSecondsPerSecond + ts_div * tick_mod + ts_mod * tick_div + value;
            }
        public:
            constexpr explicit Tick(s64 t = 0) : tick(t) { /* ... */ }
            constexpr Tick(TimeSpan ts) : tick(ConvertTimeSpanToTickImpl(ts)) { /* ... */ }

            constexpr operator s64() const { return this->tick; }

            /* Tick arithmetic. */
            constexpr Tick &operator+=(Tick rhs) { this->tick += rhs.tick; return *this; }
            constexpr Tick &operator-=(Tick rhs) { this->tick -= rhs.tick; return *this; }
            constexpr Tick operator+(Tick rhs) const { Tick r(*this); return r += rhs; }
            constexpr Tick operator-(Tick rhs) const { Tick r(*this); return r -= rhs; }

            constexpr Tick &operator+=(TimeSpan rhs) { this->tick += Tick(rhs).tick; return *this; }
            constexpr Tick &operator-=(TimeSpan rhs) { this->tick -= Tick(rhs).tick; return *this; }
            constexpr Tick operator+(TimeSpan rhs) const { Tick r(*this); return r += rhs; }
            constexpr Tick operator-(TimeSpan rhs) const { Tick r(*this); return r -= rhs; }
    };

}
