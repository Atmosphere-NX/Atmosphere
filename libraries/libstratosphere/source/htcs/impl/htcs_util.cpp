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
#include "htcs_util.hpp"

namespace ams::htcs::impl {

    s32 ConvertResultToErrorCode(const Result result) {
        /* Convert success. */
        if (R_SUCCEEDED(result)) {
            return 0;
        }

        R_TRY_CATCH(result) {
            R_CATCH(htclow::ResultNonBlockingReceiveFailed) { return HTCS_EWOULDBLOCK; }
            R_CATCH(htcs::ResultInvalidHandle)              { return HTCS_EBADF; }
            R_CATCH(htc::ResultUnknown2001)                 { return HTCS_EINVAL; }
            R_CATCH(htc::ResultUnknown2101)                 { return HTCS_EMFILE; }
            R_CATCH(htc::ResultTaskCancelled)               { return HTCS_EINTR; }
            R_CATCH(htc::ResultInvalidTaskId)               { return HTCS_EINTR; }
            R_CATCH(htc::ResultCancelled)                   { return HTCS_EINTR; }
            R_CATCH(htc::ResultTaskQueueNotAvailable)       { return HTCS_ENETDOWN; }
            R_CATCH(htclow::ResultConnectionFailure)        { return HTCS_ENETDOWN; }
            R_CATCH(htclow::ResultChannelNotExist)          { return HTCS_ENOTCONN; }
            R_CATCH_ALL()                                   { return HTCS_EUNKNOWN; }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        __builtin_unreachable();
    }

}
