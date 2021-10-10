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
#include "os_tick_manager.hpp"

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_timeout_helper_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::TimeoutHelper"
#endif

namespace ams::os::impl {

    class TimeoutHelper {
        private:
            Tick m_absolute_end_tick;
        public:
            explicit TimeoutHelper(TimeSpan timeout) {
                if (timeout == 0) {
                    /* If timeout is zero, don't do relative tick calculations. */
                    m_absolute_end_tick = Tick(0);
                } else {
                    const auto &tick_manager = impl::GetTickManager();

                    const u64 cur_tick     = tick_manager.GetTick().GetInt64Value();
                    const u64 timeout_tick = tick_manager.ConvertToTick(timeout).GetInt64Value();
                    const u64 end_tick     = cur_tick + timeout_tick + 1;

                    m_absolute_end_tick = Tick(std::min<u64>(std::numeric_limits<s64>::max(), end_tick));
                }
            }

            static void Sleep(TimeSpan tm) {
                TimeoutHelperImpl::Sleep(tm);
            }

            bool TimedOut() const {
                if (m_absolute_end_tick.GetInt64Value() == 0) {
                    return true;
                }

                const Tick cur_tick = impl::GetTickManager().GetTick();

                return cur_tick >= m_absolute_end_tick;
            }

            TargetTimeSpan GetTimeLeftOnTarget() const;
    };

}
