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
#include "os_timeout_helper_impl.os.windows.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    void TimeoutHelperImpl::Sleep(TimeSpan tm) {
        /* If asked to sleep for no time, do nothing */
        if (tm == 0) {
            return;
        }

        /* Get the end tick. */
        auto &tick_manager = GetTickManager();
        u64 tick_current = tick_manager.GetTick().GetInt64Value();
        u64 tick_timeout = tick_manager.ConvertToTick(tm).GetInt64Value();
        u64 tick_end     = tick_current + tick_timeout + 1;

        const auto end = os::Tick(std::min<u64>(std::numeric_limits<s64>::max(), tick_end));
        while (true) {
            const auto tick = tick_manager.GetTick();
            if (tick >= end) {
                return;
            }

            ::Sleep(ConvertToImplTime(end - tick));
        }
    }

}
