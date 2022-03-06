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
        AMS_UNUSED(reply_target);
        AMS_ABORT("TODO: tipc Linux Reply");
    }

    ALWAYS_INLINE Result CloseHandle(tipc::NativeHandle handle) {
        AMS_UNUSED(handle);
        AMS_ABORT("TODO: tipc Linux CloseHandle");
    }

    ALWAYS_INLINE Result CreateSession(tipc::NativeHandle *out_server_session_handle, tipc::NativeHandle *out_client_session_handle, bool is_light, uintptr_t name) {
        AMS_UNUSED(out_server_session_handle, out_client_session_handle, is_light, name);
        AMS_ABORT("TODO: tipc Linux CreateSession");
    }

    ALWAYS_INLINE Result AcceptSession(tipc::NativeHandle *out_handle, tipc::NativeHandle port) {
        AMS_UNUSED(out_handle, port);
        AMS_ABORT("TODO: tipc Linux AcceptSession");
    }

}
