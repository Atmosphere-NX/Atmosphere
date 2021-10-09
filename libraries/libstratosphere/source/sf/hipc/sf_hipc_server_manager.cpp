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
#include "sf_hipc_mitm_query_api.hpp"

namespace ams::sf::hipc {

    Result ServerManagerBase::InstallMitmServerImpl(os::NativeHandle *out_port_handle, sm::ServiceName service_name, ServerManagerBase::MitmQueryFunction query_func) {
        /* Install the Mitm. */
        os::NativeHandle query_handle;
        R_TRY(sm::mitm::InstallMitm(out_port_handle, std::addressof(query_handle), service_name));

        /* Register the query handle. */
        impl::RegisterMitmQueryHandle(query_handle, query_func);

        /* Clear future declarations if any, now that our query handler is present. */
        R_ABORT_UNLESS(sm::mitm::ClearFutureMitm(service_name));

        return ResultSuccess();
    }

    void ServerManagerBase::RegisterServerSessionToWait(ServerSession *session) {
        session->has_received = false;

        /* Set user data tag. */
        os::SetMultiWaitHolderUserData(session, static_cast<uintptr_t>(UserDataTag::Session));

        this->LinkToDeferredList(session);
    }

    void ServerManagerBase::LinkToDeferredList(os::MultiWaitHolderType *holder) {
        std::scoped_lock lk(this->deferred_list_mutex);
        os::LinkMultiWaitHolder(std::addressof(this->deferred_list), holder);
        this->notify_event.Signal();
    }

    void ServerManagerBase::LinkDeferred() {
        std::scoped_lock lk(this->deferred_list_mutex);
        os::MoveAllMultiWaitHolder(std::addressof(this->multi_wait), std::addressof(this->deferred_list));
    }

    os::MultiWaitHolderType *ServerManagerBase::WaitSignaled() {
        std::scoped_lock lk(this->selection_mutex);
        while (true) {
            this->LinkDeferred();
            auto selected = os::WaitAny(std::addressof(this->multi_wait));
            if (selected == std::addressof(this->request_stop_event_holder)) {
                return nullptr;
            } else if (selected == std::addressof(this->notify_event_holder)) {
                this->notify_event.Clear();
            } else {
                os::UnlinkMultiWaitHolder(selected);
                return selected;
            }
        }
    }

    void ServerManagerBase::ResumeProcessing() {
        this->request_stop_event.Clear();
    }

    void ServerManagerBase::RequestStopProcessing() {
        this->request_stop_event.Signal();
    }

    void ServerManagerBase::AddUserMultiWaitHolder(os::MultiWaitHolderType *holder) {
        const auto user_data_tag = static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder));
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Server);
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::MitmServer);
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Session);
        this->LinkToDeferredList(holder);
    }

    Result ServerManagerBase::ProcessForServer(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::Server);

        Server *server = static_cast<Server *>(holder);
        ON_SCOPE_EXIT { this->LinkToDeferredList(server); };

        /* Create new session. */
        if (server->static_object) {
            return this->AcceptSession(server->port_handle, server->static_object.Clone());
        } else {
            return this->OnNeedsToAccept(server->index, server);
        }
    }

    Result ServerManagerBase::ProcessForMitmServer(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::MitmServer);

        Server *server = static_cast<Server *>(holder);
        ON_SCOPE_EXIT { this->LinkToDeferredList(server); };

        /* Create resources for new session. */
        return this->OnNeedsToAccept(server->index, server);
    }

    Result ServerManagerBase::ProcessForSession(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::Session);

        ServerSession *session = static_cast<ServerSession *>(holder);

        cmif::PointerAndSize tls_message(svc::GetThreadLocalRegion()->message_buffer, hipc::TlsMessageBufferSize);
        const cmif::PointerAndSize &saved_message = session->saved_message;
        AMS_ABORT_UNLESS(tls_message.GetSize() == saved_message.GetSize());
        if (!session->has_received) {
            R_TRY(this->ReceiveRequest(session, tls_message));
            session->has_received = true;
            std::memcpy(saved_message.GetPointer(), tls_message.GetPointer(), tls_message.GetSize());
        } else {
            /* We were deferred and are re-receiving, so just memcpy. */
            std::memcpy(tls_message.GetPointer(), saved_message.GetPointer(), tls_message.GetSize());
        }

        /* Treat a meta "Context Invalidated" message as a success. */
        R_TRY_CATCH(this->ProcessRequest(session, tls_message)) {
            R_CONVERT(sf::impl::ResultRequestInvalidated, ResultSuccess());
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ServerManagerBase::Process(os::MultiWaitHolderType *holder) {
        switch (static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder))) {
            case UserDataTag::Server:
                return this->ProcessForServer(holder);
            case UserDataTag::MitmServer:
                return this->ProcessForMitmServer(holder);
            case UserDataTag::Session:
                return this->ProcessForSession(holder);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool ServerManagerBase::WaitAndProcessImpl() {
        if (auto *signaled_holder = this->WaitSignaled(); signaled_holder != nullptr) {
            R_ABORT_UNLESS(this->Process(signaled_holder));
            return true;
        } else {
            return false;
        }
    }

    void ServerManagerBase::WaitAndProcess() {
        this->WaitAndProcessImpl();
    }

    void ServerManagerBase::LoopProcess() {
        while (this->WaitAndProcessImpl()) {
            /* ... */
        }
    }

}
