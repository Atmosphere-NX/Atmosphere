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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        ALWAYS_INLINE Result SendSyncRequestLight(ams::svc::Handle session_handle, u32 *args) {
            /* Get the light client session from its handle. */
            KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KLightClientSession>(session_handle);
            R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

            /* Send the request. */
            R_TRY(session->SendSyncRequest(args));

            return ResultSuccess();
        }

        ALWAYS_INLINE Result ReplyAndReceiveLight(ams::svc::Handle session_handle, u32 *args) {
            /* Get the light server session from its handle. */
            KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KLightServerSession>(session_handle);
            R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

            /* Handle the request. */
            R_TRY(session->ReplyAndReceive(args));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SendSyncRequestLight64(ams::svc::Handle session_handle, u32 *args) {
        return SendSyncRequestLight(session_handle, args);
    }

    Result ReplyAndReceiveLight64(ams::svc::Handle session_handle, u32 *args) {
        return ReplyAndReceiveLight(session_handle, args);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SendSyncRequestLight64From32(ams::svc::Handle session_handle, u32 *args) {
        return SendSyncRequestLight(session_handle, args);
    }

    Result ReplyAndReceiveLight64From32(ams::svc::Handle session_handle, u32 *args) {
        return ReplyAndReceiveLight(session_handle, args);
    }

}
