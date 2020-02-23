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



    }

    /* =============================    64 ABI    ============================= */

    Result SendSyncRequest64(ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequest64 was called.");
    }

    Result SendSyncRequestWithUserBuffer64(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequestWithUserBuffer64 was called.");
    }

    Result SendAsyncRequestWithUserBuffer64(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendAsyncRequestWithUserBuffer64 was called.");
    }

    Result ReplyAndReceive64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceive64 was called.");
    }

    Result ReplyAndReceiveWithUserBuffer64(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceiveWithUserBuffer64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result SendSyncRequest64From32(ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequest64From32 was called.");
    }

    Result SendSyncRequestWithUserBuffer64From32(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequestWithUserBuffer64From32 was called.");
    }

    Result SendAsyncRequestWithUserBuffer64From32(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendAsyncRequestWithUserBuffer64From32 was called.");
    }

    Result ReplyAndReceive64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceive64From32 was called.");
    }

    Result ReplyAndReceiveWithUserBuffer64From32(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceiveWithUserBuffer64From32 was called.");
    }

}
