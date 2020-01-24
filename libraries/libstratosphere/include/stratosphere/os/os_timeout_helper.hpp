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
#include "os_common_types.hpp"

namespace ams::os {

    class TimeoutHelper {
        private:
            u64 end_tick;
        public:
            TimeoutHelper(u64 ns) {
                /* Special case zero-time timeouts. */
                if (ns == 0) {
                    end_tick = 0;
                    return;
                }

                u64 cur_tick = armGetSystemTick();
                this->end_tick = cur_tick + NsToTick(ns) + 1;
            }

            static constexpr inline u64 NsToTick(u64 ns) {
                return (ns * 12) / 625;
            }

            static constexpr inline u64 TickToNs(u64 tick) {
                return (tick * 625) / 12;
            }

            inline bool TimedOut() const {
                if (this->end_tick == 0) {
                    return true;
                }

                return armGetSystemTick() >= this->end_tick;
            }

            inline u64 NsUntilTimeout() const {
                u64 diff = TickToNs(this->end_tick - armGetSystemTick());

                if (this->TimedOut()) {
                    return 0;
                }

                return diff;
            }
    };

}
