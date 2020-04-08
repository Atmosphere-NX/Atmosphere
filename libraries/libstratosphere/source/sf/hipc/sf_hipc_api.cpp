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

namespace ams::sf::hipc {

    namespace {

        ALWAYS_INLINE Result ReceiveImpl(Handle session_handle, void *message_buf, size_t message_buf_size) {
            s32 unused_index;
            if (message_buf == armGetTls()) {
                /* Consider: AMS_ABORT_UNLESS(message_buf_size == TlsMessageBufferSize); */
                return svcReplyAndReceive(&unused_index, &session_handle, 1, INVALID_HANDLE, std::numeric_limits<u64>::max());
            } else {
                return svcReplyAndReceiveWithUserBuffer(&unused_index, message_buf, message_buf_size, &session_handle, 1, INVALID_HANDLE, std::numeric_limits<u64>::max());
            }
        }

        ALWAYS_INLINE Result ReplyImpl(Handle session_handle, void *message_buf, size_t message_buf_size) {
            s32 unused_index;
            if (message_buf == armGetTls()) {
                /* Consider: AMS_ABORT_UNLESS(message_buf_size == TlsMessageBufferSize); */
                return svcReplyAndReceive(&unused_index, &session_handle, 0, session_handle, 0);
            } else {
                return svcReplyAndReceiveWithUserBuffer(&unused_index, message_buf, message_buf_size, &session_handle, 0, session_handle, 0);
            }
        }

    }

    void AttachWaitableHolderForAccept(os::WaitableHolderType *holder, Handle port) {
        return os::InitializeWaitableHolder(holder, port);
    }

    void AttachWaitableHolderForReply(os::WaitableHolderType *holder, Handle request) {
        return os::InitializeWaitableHolder(holder, request);
    }

    Result Receive(ReceiveResult *out_recv_result, Handle session_handle, const cmif::PointerAndSize &message_buffer) {
        R_TRY_CATCH(ReceiveImpl(session_handle, message_buffer.GetPointer(), message_buffer.GetSize())) {
            R_CATCH(svc::ResultSessionClosed) {
                *out_recv_result = ReceiveResult::Closed;
                return ResultSuccess();
            }
            R_CATCH(svc::ResultReceiveListBroken) {
                *out_recv_result = ReceiveResult::NeedsRetry;
                return ResultSuccess();
            }
        } R_END_TRY_CATCH;
        *out_recv_result = ReceiveResult::Success;
        return ResultSuccess();
    }

    Result Receive(bool *out_closed, Handle session_handle, const cmif::PointerAndSize &message_buffer) {
        R_TRY_CATCH(ReceiveImpl(session_handle, message_buffer.GetPointer(), message_buffer.GetSize())) {
            R_CATCH(svc::ResultSessionClosed) {
                *out_closed = true;
                return ResultSuccess();
            }
        } R_END_TRY_CATCH;
        *out_closed = false;
        return ResultSuccess();
    }

    Result Reply(Handle session_handle, const cmif::PointerAndSize &message_buffer) {
        R_TRY_CATCH(ReplyImpl(session_handle, message_buffer.GetPointer(), message_buffer.GetSize())) {
            R_CONVERT(svc::ResultTimedOut,      ResultSuccess())
            R_CONVERT(svc::ResultSessionClosed, ResultSuccess())
        } R_END_TRY_CATCH;
        /* ReplyImpl should *always* return an error. */
        AMS_ABORT_UNLESS(false);
    }

    Result CreateSession(Handle *out_server_handle, Handle *out_client_handle) {
        R_TRY_CATCH(svcCreateSession(out_server_handle, out_client_handle, 0, 0)) {
            R_CONVERT(svc::ResultOutOfResource, sf::hipc::ResultOutOfSessions());
        } R_END_TRY_CATCH;
        return ResultSuccess();
    }

}
