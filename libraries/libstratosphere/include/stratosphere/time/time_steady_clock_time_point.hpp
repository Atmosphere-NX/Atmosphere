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
#include <stratosphere/time/time_common.hpp>

namespace ams::time {

    struct SteadyClockTimePoint {
        s64 value;
        SourceId source_id;

        constexpr SteadyClockTimePoint &operator+=(const TimeSpan &ts) { this->value += ts.GetSeconds(); return *this; }
        constexpr SteadyClockTimePoint &operator-=(const TimeSpan &ts) { this->value -= ts.GetSeconds(); return *this; }

        constexpr friend SteadyClockTimePoint operator+(const SteadyClockTimePoint &lhs, const TimeSpan &rhs) { return { .value = lhs.value + rhs.GetSeconds(), .source_id = lhs.source_id }; }
        constexpr friend SteadyClockTimePoint operator-(const SteadyClockTimePoint &lhs, const TimeSpan &rhs) { return { .value = lhs.value - rhs.GetSeconds(), .source_id = lhs.source_id }; }

        constexpr friend bool operator==(const SteadyClockTimePoint &lhs, const SteadyClockTimePoint &rhs) { return lhs.value == rhs.value && lhs.source_id == rhs.source_id; }
        constexpr friend bool operator!=(const SteadyClockTimePoint &lhs, const SteadyClockTimePoint &rhs) { return !(lhs == rhs); }
    };

}
