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
#include <stratosphere.hpp>

namespace ams::powctl::impl {

    constexpr inline const TimeSpan PowerControlRetryTimeout  = TimeSpan::FromSeconds(10);
    constexpr inline const TimeSpan PowerControlRetryInterval = TimeSpan::FromMilliSeconds(20);

    #define AMS_POWCTL_R_TRY_WITH_RETRY(__EXPR__)                                                        \
        ({                                                                                               \
            TimeSpan __powctl_retry_current_time = 0;                                                    \
            while (true) {                                                                               \
                const Result __powctl_retry_result = (__EXPR__);                                         \
                if (R_SUCCEEDED(__powctl_retry_result)) {                                                \
                    break;                                                                               \
                }                                                                                        \
                                                                                                         \
                __powctl_retry_current_time += PowerControlRetryInterval;                                \
                R_UNLESS(__powctl_retry_current_time < PowerControlRetryTimeout, __powctl_retry_result); \
                                                                                                         \
                os::SleepThread(PowerControlRetryInterval);                                              \
            }                                                                                            \
        })


}
