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
#include "os_timeout_helper_impl.os.linux.hpp"

#include <time.h>

namespace ams::os::impl {

    void TimeoutHelperImpl::Sleep(TimeSpan tm) {
        /* If asked to sleep for no time, do nothing */
        if (tm == 0) {
            return;
        }

        constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
        const s64 ns = tm.GetNanoSeconds();
        struct timespec ts = { .tv_sec = (ns / NanoSecondsPerSecond), .tv_nsec = ns % NanoSecondsPerSecond };

        while (true) {
            const auto ret = ::nanosleep(std::addressof(ts), std::addressof(ts));
            if (ret == 0) {
                break;
            }

            AMS_ASSERT(errno == EINTR);
        }
    }

}
