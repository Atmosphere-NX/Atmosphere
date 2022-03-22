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
#include "os_multiple_wait_holder_base.hpp"
#include "os_multiple_wait_impl.hpp"

namespace ams::os::impl {

    Result MultiWaitHorizonImpl::WaitSynchronizationN(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns) {
        AMS_UNUSED(array_size);

        AMS_ASSERT(!(num == 0 && ns == 0));
        s32 index = MultiWaitImpl::WaitInvalid;

        R_TRY_CATCH(svc::WaitSynchronization(std::addressof(index), static_cast<const svc::Handle *>(arr), num, ns)) {
            R_CATCH(svc::ResultTimedOut)  { index = MultiWaitImpl::WaitTimedOut; }
            R_CATCH(svc::ResultCancelled) { index = MultiWaitImpl::WaitCancelled; }
            /* All other results are critical errors. */
            /* svc::ResultThreadTerminating */
            /* svc::ResultInvalidHandle. */
            /* svc::ResultInvalidPointer */
            /* svc::ResultOutOfRange */
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        *out_index = index;
        return ResultSuccess();
    }

    Result MultiWaitHorizonImpl::ReplyAndReceiveN(s32 *out_index, s32 num, NativeHandle arr[], s32 array_size, s64 ns, NativeHandle reply_target) {
        AMS_UNUSED(array_size);

        /* NOTE: Nintendo does not initialize this value, which seems like it can cause incorrect behavior. */
        s32 index = MultiWaitImpl::WaitInvalid;
        static_assert(MultiWaitImpl::WaitInvalid != -1);

        R_TRY_CATCH(svc::ReplyAndReceive(std::addressof(index), arr, num, reply_target, ns)) {
            R_CATCH(svc::ResultTimedOut)  { *out_index = MultiWaitImpl::WaitTimedOut;  return R_CURRENT_RESULT; }
            R_CATCH(svc::ResultCancelled) { *out_index = MultiWaitImpl::WaitCancelled; return R_CURRENT_RESULT; }
            R_CATCH(svc::ResultSessionClosed)   {
                if (index == -1) {
                    *out_index = MultiWaitImpl::WaitInvalid;
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

        *out_index = index;
        return ResultSuccess();
    }

    void MultiWaitHorizonImpl::CancelWait() {
        R_ABORT_UNLESS(svc::CancelSynchronization(m_handle));
    }

}
