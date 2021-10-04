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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/cmif/sf_cmif_pointer_and_size.hpp>

namespace ams::sf::hipc {

    constexpr size_t TlsMessageBufferSize = 0x100;

    enum class ReceiveResult {
        Success,
        Closed,
        NeedsRetry,
    };

    void AttachMultiWaitHolderForAccept(os::MultiWaitHolderType *holder, os::NativeHandle port);
    void AttachMultiWaitHolderForReply(os::MultiWaitHolderType *holder, os::NativeHandle request);

    Result Receive(ReceiveResult *out_recv_result, os::NativeHandle session_handle, const cmif::PointerAndSize &message_buffer);
    Result Receive(bool *out_closed, os::NativeHandle session_handle, const cmif::PointerAndSize &message_buffer);
    Result Reply(os::NativeHandle session_handle, const cmif::PointerAndSize &message_buffer);

    Result CreateSession(os::NativeHandle *out_server_handle, os::NativeHandle *out_client_handle);

}
