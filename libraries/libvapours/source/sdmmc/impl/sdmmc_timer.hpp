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
#include <vapours.hpp>

namespace ams::sdmmc::impl {

    void WaitMicroSeconds(u32 us);
    void WaitClocks(u32 num_clocks, u32 clock_frequency_khz);

    #if defined(AMS_SDMMC_USE_OS_TIMER)
        class ManualTimer {
            private:
                os::Tick timeout_tick;
                bool is_timed_out;
            public:
                explicit ManualTimer(u32 ms) {
                    this->timeout_tick = os::GetSystemTick() + os::ConvertToTick(TimeSpan::FromMilliSeconds(ms));
                    this->is_timed_out = false;
                }

                bool Update() {
                    if (this->is_timed_out) {
                        return false;
                    }

                    this->is_timed_out = os::GetSystemTick() > this->timeout_tick;
                    return true;
                }
        };
    #elif defined(AMS_SDMMC_USE_UTIL_TIMER)
        class ManualTimer {
            private:
                u32 timeout_us;
                bool is_timed_out;
            public:
                explicit ManualTimer(u32 ms) {
                    this->timeout_us   = util::GetMicroSeconds() + (ms * 1000);
                    this->is_timed_out = false;
                }

                bool Update() {
                    if (this->is_timed_out) {
                        return false;
                    }

                    this->is_timed_out = util::GetMicroSeconds() > this->timeout_us;
                    return true;
                }
        };
    #else
        #error "Unknown context for ams::sdmmc::ManualTimer"
    #endif

}
