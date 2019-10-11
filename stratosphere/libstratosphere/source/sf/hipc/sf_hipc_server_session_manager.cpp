/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace sts::sf::hipc {

    void ServerSessionManager::DestroySession(ServerSession *session) {
        /* Destroy object. */
        session->~ServerSession();
        /* Free object memory. */
        this->FreeSession(session);
    }

    void ServerSessionManager::CloseSessionImpl(ServerSession *session) {
        const Handle session_handle = session->session_handle;
        this->DestroySession(session);
        R_ASSERT(svcCloseHandle(session_handle));
    }

    Result ServerSessionManager::RegisterSessionImpl(ServerSession *session_memory, Handle session_handle, cmif::ServiceObjectHolder &&obj) {
        /* Create session object. */
        new (session_memory) ServerSession(session_handle, std::forward<cmif::ServiceObjectHolder>(obj));
        /* Assign session resources. */
        session_memory->pointer_buffer = this->GetSessionPointerBuffer(session_memory);
        session_memory->saved_message  = this->GetSessionSavedMessageBuffer(session_memory);
        /* Register to wait list. */
        this->RegisterSessionToWaitList(session_memory);
        return ResultSuccess;
    }

    Result ServerSessionManager::AcceptSessionImpl(ServerSession *session_memory, Handle port_handle, cmif::ServiceObjectHolder &&obj) {
        /* Create session handle. */
        Handle session_handle;
        R_TRY(svcAcceptSession(&session_handle, port_handle));
        bool succeeded = false;
        ON_SCOPE_EXIT {
            if (!succeeded) {
                R_ASSERT(svcCloseHandle(session_handle));
            }
        };
        /* Register session. */
        R_TRY(this->RegisterSessionImpl(session_memory, session_handle, std::forward<cmif::ServiceObjectHolder>(obj)));
        succeeded = true;
        return ResultSuccess;
    }

    Result ServerSessionManager::RegisterMitmSessionImpl(ServerSession *session_memory, Handle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
        /* Create session object. */
        new (session_memory) ServerSession(mitm_session_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv));
        /* Assign session resources. */
        session_memory->pointer_buffer = this->GetSessionPointerBuffer(session_memory);
        session_memory->saved_message  = this->GetSessionSavedMessageBuffer(session_memory);
        /* Register to wait list. */
        this->RegisterSessionToWaitList(session_memory);
        return ResultSuccess;
    }

    Result ServerSessionManager::AcceptMitmSessionImpl(ServerSession *session_memory, Handle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
        /* Create session handle. */
        Handle mitm_session_handle;
        R_TRY(svcAcceptSession(&mitm_session_handle, mitm_port_handle));
        bool succeeded = false;
        ON_SCOPE_EXIT {
            if (!succeeded) {
                R_ASSERT(svcCloseHandle(mitm_session_handle));
            }
        };
        /* Register session. */
        R_TRY(this->RegisterMitmSessionImpl(session_memory, mitm_session_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv)));
        succeeded = true;
        return ResultSuccess;
    }

    Result ServerSessionManager::RegisterSession(Handle session_handle, cmif::ServiceObjectHolder &&obj) {
        /* We don't actually care about what happens to the session. It'll get linked. */
        ServerSession *session_ptr = nullptr;
        return this->RegisterSession(&session_ptr, session_handle,std::forward<cmif::ServiceObjectHolder>(obj));
    }

    Result ServerSessionManager::AcceptSession(Handle port_handle, cmif::ServiceObjectHolder &&obj) {
        /* We don't actually care about what happens to the session. It'll get linked. */
        ServerSession *session_ptr = nullptr;
        return this->AcceptSession(&session_ptr, port_handle, std::forward<cmif::ServiceObjectHolder>(obj));
    }

    Result ServerSessionManager::RegisterMitmSession(Handle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
        /* We don't actually care about what happens to the session. It'll get linked. */
        ServerSession *session_ptr = nullptr;
        return this->RegisterMitmSession(&session_ptr, mitm_session_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv));
    }

    Result ServerSessionManager::AcceptMitmSession(Handle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
        /* We don't actually care about what happens to the session. It'll get linked. */
        ServerSession *session_ptr = nullptr;
        return this->AcceptMitmSession(&session_ptr, mitm_port_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv));
    }

    Result ServerSessionManager::ReceiveRequestImpl(ServerSession *session, const cmif::PointerAndSize &message) {
        const cmif::PointerAndSize &pointer_buffer = session->pointer_buffer;

        /* If the receive list is odd, we may need to receive repeatedly. */
        while (true) {
            if (pointer_buffer.GetPointer()) {
                hipcMakeRequestInline(message.GetPointer(),
                    .type = CmifCommandType_Invalid,
                    .num_recv_statics = HIPC_AUTO_RECV_STATIC,
                ).recv_list[0] = hipcMakeRecvStatic(pointer_buffer.GetPointer(), pointer_buffer.GetSize());
            } else {
                hipcMakeRequestInline(message.GetPointer(),
                    .type = CmifCommandType_Invalid,
                );
            }
            hipc::ReceiveResult recv_result;
            R_TRY(hipc::Receive(&recv_result, session->session_handle, message));
            switch (recv_result) {
                case hipc::ReceiveResult::Success:
                    session->is_closed = false;
                    return ResultSuccess;
                case hipc::ReceiveResult::Closed:
                    session->is_closed = true;
                    return ResultSuccess;
                case hipc::ReceiveResult::NeedsRetry:
                    continue;
                STS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    namespace {

        NX_CONSTEXPR u32 GetCmifCommandType(const cmif::PointerAndSize &message) {
            HipcHeader hdr = {};
            __builtin_memcpy(&hdr, message.GetPointer(), sizeof(hdr));
            return hdr.type;
        }

    }

    Result ServerSessionManager::ProcessRequest(ServerSession *session, const cmif::PointerAndSize &message) {
        if (session->is_closed) {
            this->CloseSessionImpl(session);
            return ResultSuccess;
        }
        switch (GetCmifCommandType(message)) {
            case CmifCommandType_Close:
            {
                this->CloseSessionImpl(session);
                return ResultSuccess;
            }
            default:
            {
                R_TRY_CATCH(this->ProcessRequestImpl(session, message, message)) {
                    R_CATCH(ResultServiceFrameworkRequestDeferredByUser) { /* TODO: Properly include entire range Nintendo does */
                        return ResultServiceFrameworkRequestDeferredByUser;
                    }
                    R_CATCH_ALL() {
                        /* All other results indicate something went very wrong. */
                        this->CloseSessionImpl(session);
                        return ResultSuccess;
                    }
                } R_END_TRY_CATCH;
                /* We succeeded, so we can process future messages on this session. */
                this->RegisterSessionToWaitList(session);
                return ResultSuccess;
            }
        }
    }

    Result ServerSessionManager::ProcessRequestImpl(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* TODO: Inline context support, retrieve from raw data + 0xC. */
        const auto cmif_command_type = GetCmifCommandType(in_message);
        switch (cmif_command_type) {
            case CmifCommandType_Request:
            case CmifCommandType_RequestWithContext:
                return this->DispatchRequest(session->srv_obj_holder.Clone(), session, in_message, out_message);
            case CmifCommandType_Control:
            case CmifCommandType_ControlWithContext:
                return this->DispatchManagerRequest(session, in_message, out_message);
            default:
                return ResultHipcUnknownCommandType;
        }
    }

    Result ServerSessionManager::DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* This will get overridden by ... WithDomain class. */
        return ResultServiceFrameworkNotSupported;
    }

    Result ServerSessionManager::DispatchRequest(cmif::ServiceObjectHolder &&obj_holder, ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* Create request context. */
        cmif::HandlesToClose handles_to_close = {};
        cmif::ServiceDispatchContext dispatch_ctx = {
            .srv_obj = obj_holder.GetServiceObjectUnsafe(),
            .manager = this,
            .processor = nullptr, /* Filled in by template implementations. */
            .handles_to_close = &handles_to_close,
            .pointer_buffer = session->pointer_buffer,
            .in_message_buffer = in_message,
            .out_message_buffer = out_message,
            .request = hipcParseRequest(in_message.GetPointer()),
        };

        /* Validate message sizes. */
        const uintptr_t in_message_buffer_end = in_message.GetAddress() + in_message.GetSize();
        const uintptr_t in_raw_addr = reinterpret_cast<uintptr_t>(dispatch_ctx.request.data.data_words);
        const size_t in_raw_size = dispatch_ctx.request.meta.num_data_words * sizeof(u32);
        /* Note: Nintendo does not validate this size before subtracting 0x10 from it. This is not exploitable. */
        R_UNLESS(in_raw_size >= 0x10, ResultHipcInvalidRequestSize);
        R_UNLESS(in_raw_addr + in_raw_size <= in_message_buffer_end, ResultHipcInvalidRequestSize);
        const uintptr_t recv_list_end = reinterpret_cast<uintptr_t>(dispatch_ctx.request.data.recv_list + dispatch_ctx.request.meta.num_recv_statics);
        R_UNLESS(recv_list_end <= in_message_buffer_end, ResultHipcInvalidRequestSize);

        /* CMIF has 0x10 of padding in raw data, and requires 0x10 alignment. */
        const cmif::PointerAndSize in_raw_data(util::AlignUp(in_raw_addr, 0x10), in_raw_size - 0x10);

        /* Invoke command handler. */
        R_TRY(obj_holder.ProcessMessage(dispatch_ctx, in_raw_data));

        /* Reply. */
        {
            ON_SCOPE_EXIT {
                for (size_t i = 0; i < handles_to_close.num_handles; i++) {
                    R_ASSERT(svcCloseHandle(handles_to_close.handles[i]));
                }
            };
            R_TRY(hipc::Reply(session->session_handle, out_message));
        }

        return ResultSuccess;
    }


}
