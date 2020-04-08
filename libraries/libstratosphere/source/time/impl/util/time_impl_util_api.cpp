/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::time::impl::util {

    Result GetSpanBetween(s64 *out, const SteadyClockTimePoint &from, const SteadyClockTimePoint &to) {
        AMS_ASSERT(out != nullptr);

        R_UNLESS(out != nullptr,                 time::ResultInvalidPointer());
        R_UNLESS(from.source_id == to.source_id, time::ResultNotComparable());

        const bool no_overflow = (from.value >= 0 ? (to.value >= std::numeric_limits<s64>::min() + from.value)
                                                  : (to.value <= std::numeric_limits<s64>::max() + from.value));
        R_UNLESS(no_overflow, time::ResultOverflowed());

        *out = to.value - from.value;
        return ResultSuccess();
    }

}
