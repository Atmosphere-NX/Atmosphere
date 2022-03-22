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
#include <stratosphere.hpp>
#include "os_timeout_helper.hpp"

namespace ams::os::impl {

    TargetTimeSpan TimeoutHelper::GetTimeLeftOnTarget() const {
        /* If the absolute tick is zero, we're expired. */
        if (m_absolute_end_tick.GetInt64Value() == 0) {
            return 0;
        }

        /* Check if we've expired. */
        const Tick cur_tick = impl::GetTickManager().GetTick();
        if (cur_tick >= m_absolute_end_tick) {
            return 0;
        }

        /* Return the converted difference as a timespan. */
        return TimeoutHelperImpl::ConvertToImplTime(m_absolute_end_tick - cur_tick);
    }

}
