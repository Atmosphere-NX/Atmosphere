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

namespace ams::os::impl {

    class CheckBusyMutexPermission {
        public:
            CheckBusyMutexPermission() {
                /* In order to use the disable counter, we must support SynchronizePreemptionState. */
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_IsSvcPermitted, 0, svc::SvcId_SynchronizePreemptionState));

                /* Verify that it's supported. */
                AMS_ABORT_UNLESS(value != 0);
            }
    };

    ALWAYS_INLINE void CallCheckBusyMutexPermission() {
        AMS_FUNCTION_LOCAL_STATIC(CheckBusyMutexPermission, s_check);
        AMS_UNUSED(s_check);
    }

}
