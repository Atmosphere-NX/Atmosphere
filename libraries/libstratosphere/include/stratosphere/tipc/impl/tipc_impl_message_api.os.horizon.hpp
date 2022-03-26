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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_message_types.hpp>

namespace ams::tipc::impl {

    inline void Reply(tipc::NativeHandle reply_target) {
        /* Perform the reply. */
        s32 dummy;
        R_TRY_CATCH(svc::ReplyAndReceive(std::addressof(dummy), nullptr, 0, reply_target, 0)) {
            R_CATCH(svc::ResultTimedOut) {
                /* Timing out is acceptable. */
            }
            R_CATCH(svc::ResultSessionClosed) {
                /* It's okay if we couldn't reply to a closed session. */
            }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
    }

    ALWAYS_INLINE Result CloseHandle(tipc::NativeHandle handle) {
        R_RETURN(svc::CloseHandle(handle));
    }

    ALWAYS_INLINE Result CreateSession(tipc::NativeHandle *out_server_session_handle, tipc::NativeHandle *out_client_session_handle, bool is_light, uintptr_t name) {
        R_RETURN(svc::CreateSession(out_server_session_handle, out_client_session_handle, is_light, name));
    }

    ALWAYS_INLINE Result AcceptSession(tipc::NativeHandle *out_handle, tipc::NativeHandle port) {
        R_RETURN(svc::AcceptSession(out_handle, port));
    }

}
