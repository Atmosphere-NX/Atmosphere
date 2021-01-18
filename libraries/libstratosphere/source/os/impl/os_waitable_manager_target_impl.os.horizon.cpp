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

    Result WaitableManagerHorizonImpl::WaitSynchronizationN(s32 *out_index, s32 num, Handle arr[], s32 array_size, s64 ns) {
        AMS_ASSERT(!(num == 0 && ns == 0));
        s32 index = WaitableManagerImpl::WaitInvalid;

        R_TRY_CATCH(svc::WaitSynchronization(std::addressof(index), static_cast<const svc::Handle *>(arr), num, ns)) {
            R_CATCH(svc::ResultTimedOut)  { index = WaitableManagerImpl::WaitTimedOut; }
            R_CATCH(svc::ResultCancelled) { index = WaitableManagerImpl::WaitCancelled; }
            /* All other results are critical errors. */
            /* svc::ResultThreadTerminating */
            /* svc::ResultInvalidHandle. */
            /* svc::ResultInvalidPointer */
            /* svc::ResultOutOfRange */
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out_index = index;
        return ResultSuccess();
    }

    Result WaitableManagerHorizonImpl::ReplyAndReceiveN(s32 *out_index, s32 num, Handle arr[], s32 array_size, s64 ns, Handle reply_target) {
        /* NOTE: Nintendo does not initialize this value, which seems like it can cause incorrect behavior. */
        s32 index = WaitableManagerImpl::WaitInvalid;
        static_assert(WaitableManagerImpl::WaitInvalid != -1);

        R_TRY_CATCH(svc::ReplyAndReceive(std::addressof(index), arr, num, ns, reply_target)) {
            R_CATCH(svc::ResultTimedOut)  { *out_index = WaitableManagerImpl::WaitTimedOut;  return R_CURRENT_RESULT; }
            R_CATCH(svc::ResultCancelled) { *out_index = WaitableManagerImpl::WaitCancelled; return R_CURRENT_RESULT; }
            R_CATCH(svc::ResultSessionClosed)   {
                if (index == -1) {
                    *out_index = WaitableManagerImpl::WaitInvalid;
                    return os::ResultSessionClosedForReply();
                } else {
                    *out_index = index;
                    return os::ResultSessionClosedForReceive();
                }
            }
            R_CATCH(svc::ResultReceiveListBroken) {
                *out_index = index;
                return os::ResultReceiveListBroken();
            }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return ResultSuccess();
    }

    void WaitableManagerHorizonImpl::CancelWait() {
        R_ABORT_UNLESS(svc::CancelSynchronization(this->handle));
    }

}
