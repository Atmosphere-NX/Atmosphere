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
#include <stratosphere/os/os_common_types.hpp>

namespace ams::os {

    class Tick;

    /* Tick API. */
    Tick GetSystemTick();
    Tick GetSystemTickOrdered();
    s64 GetSystemTickFrequency();
    TimeSpan ConvertToTimeSpan(Tick tick);
    Tick ConvertToTick(TimeSpan ts);

    class Tick {
        private:
            s64 m_tick;
        public:
            constexpr explicit Tick(s64 t = 0) : m_tick(t) { /* ... */ }
            Tick(TimeSpan ts) : m_tick(ConvertToTick(ts).GetInt64Value()) { /* ... */ }
        public:
            constexpr s64 GetInt64Value() const { return m_tick; }
            TimeSpan ToTimeSpan() const { return ConvertToTimeSpan(*this); }

            /* Tick arithmetic. */
            constexpr Tick &operator+=(Tick rhs) { m_tick += rhs.m_tick; return *this; }
            constexpr Tick &operator-=(Tick rhs) { m_tick -= rhs.m_tick; return *this; }
            constexpr Tick operator+(Tick rhs) const { Tick r(*this); return r += rhs; }
            constexpr Tick operator-(Tick rhs) const { Tick r(*this); return r -= rhs; }

            constexpr bool operator==(const Tick &rhs) const {
                return m_tick == rhs.m_tick;
            }

            constexpr bool operator!=(const Tick &rhs) const {
                return !(*this == rhs);
            }

            constexpr bool operator<(const Tick &rhs) const {
                return m_tick < rhs.m_tick;
            }

            constexpr bool operator>=(const Tick &rhs) const {
                return !(*this < rhs);
            }

            constexpr bool operator>(const Tick &rhs) const {
                return m_tick > rhs.m_tick;
            }

            constexpr bool operator<=(const Tick &rhs) const {
                return !(*this > rhs);
            }
    };

}
