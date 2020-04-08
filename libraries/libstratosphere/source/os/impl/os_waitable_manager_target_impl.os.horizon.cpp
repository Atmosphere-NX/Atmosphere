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
#include <stratosphere.hpp>
#include "os_waitable_holder_base.hpp"
#include "os_waitable_manager_impl.hpp"

namespace ams::os::impl {

    s32 WaitableManagerHorizonImpl::WaitSynchronizationN(s32 num, Handle arr[], s32 array_size, s64 ns) {
        AMS_ASSERT(!(num == 0 && ns == 0));
        s32 index = WaitableManagerImpl::WaitInvalid;

        R_TRY_CATCH(svc::WaitSynchronization(std::addressof(index), static_cast<const svc::Handle *>(arr), num, ns)) {
            R_CATCH(svc::ResultTimedOut)  { return WaitableManagerImpl::WaitTimedOut; }
            R_CATCH(svc::ResultCancelled) { return WaitableManagerImpl::WaitCancelled; }
            /* All other results are critical errors. */
            /* svc::ResultThreadTerminating */
            /* svc::ResultInvalidHandle. */
            /* svc::ResultInvalidPointer */
            /* svc::ResultOutOfRange */
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return index;
    }

    void WaitableManagerHorizonImpl::CancelWait() {
        R_ABORT_UNLESS(svc::CancelSynchronization(this->handle));
    }

}
