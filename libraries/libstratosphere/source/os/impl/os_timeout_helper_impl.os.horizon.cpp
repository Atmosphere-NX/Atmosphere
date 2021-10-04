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
#include "os_timeout_helper_impl.os.horizon.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    void TimeoutHelperImpl::Sleep(TimeSpan tm) {
        if (tm == TimeSpan(0)) {
            GetThreadManager().YieldThread();
        } else {
            ams::svc::SleepThread(tm.GetNanoSeconds());
        }
    }

}
