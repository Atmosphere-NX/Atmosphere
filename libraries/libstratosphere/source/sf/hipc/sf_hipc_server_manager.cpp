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

    #if AMS_SF_MITM_SUPPORTED
    Result ServerManagerBase::InstallMitmServerImpl(os::NativeHandle *out_port_handle, sm::ServiceName service_name, ServerManagerBase::MitmQueryFunction query_func) {
        /* Install the Mitm. */
        os::NativeHandle query_handle = os::InvalidNativeHandle;
        R_TRY(sm::mitm::InstallMitm(out_port_handle, std::addressof(query_handle), service_name));

        /* Register the query handle. */
        impl::RegisterMitmQueryHandle(query_handle, query_func);

        /* Clear future declarations if any, now that our query handler is present. */
        R_ABORT_UNLESS(sm::mitm::ClearFutureMitm(service_name));

        R_SUCCEED();
    }
    #endif

    void ServerManagerBase::RegisterServerSessionToWait(ServerSession *session) {
        session->m_has_received = false;

        /* Set user data tag. */
        os::SetMultiWaitHolderUserData(session, static_cast<uintptr_t>(UserDataTag::Session));

        this->LinkToDeferredList(session);
    }

    void ServerManagerBase::LinkToDeferredList(os::MultiWaitHolderType *holder) {
        std::scoped_lock lk(m_deferred_list_mutex);
        os::LinkMultiWaitHolder(std::addressof(m_deferred_list), holder);
        m_notify_event.Signal();
    }

    void ServerManagerBase::LinkDeferred() {
        std::scoped_lock lk(m_deferred_list_mutex);
        os::MoveAllMultiWaitHolder(std::addressof(m_multi_wait), std::addressof(m_deferred_list));
    }

    os::MultiWaitHolderType *ServerManagerBase::WaitSignaled() {
        std::scoped_lock lk(m_selection_mutex);
        while (true) {
            this->LinkDeferred();
            auto selected = os::WaitAny(std::addressof(m_multi_wait));
            if (selected == std::addressof(m_request_stop_event_holder)) {
                return nullptr;
            } else if (selected == std::addressof(m_notify_event_holder)) {
                m_notify_event.Clear();
            } else {
                os::UnlinkMultiWaitHolder(selected);
                return selected;
            }
        }
    }

    void ServerManagerBase::ResumeProcessing() {
        m_request_stop_event.Clear();
    }

    void ServerManagerBase::RequestStopProcessing() {
        m_request_stop_event.Signal();
    }

    void ServerManagerBase::AddUserMultiWaitHolder(os::MultiWaitHolderType *holder) {
        const auto user_data_tag = static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder));
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Server);
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Session);
        #if AMS_SF_MITM_SUPPORTED
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::MitmServer);
        #endif
        this->LinkToDeferredList(holder);
    }

    Result ServerManagerBase::ProcessForServer(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::Server);

        Server *server = static_cast<Server *>(holder);
        ON_SCOPE_EXIT { this->LinkToDeferredList(server); };

        /* Create new session. */
        if (server->m_static_object) {
            R_RETURN(this->AcceptSession(server->m_port_handle, server->m_static_object.Clone()));
        } else {
            R_RETURN(this->OnNeedsToAccept(server->m_index, server));
        }
    }

    #if AMS_SF_MITM_SUPPORTED
    Result ServerManagerBase::ProcessForMitmServer(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::MitmServer);

        Server *server = static_cast<Server *>(holder);
        ON_SCOPE_EXIT { this->LinkToDeferredList(server); };

        /* Create resources for new session. */
        R_RETURN(this->OnNeedsToAccept(server->m_index, server));
    }
    #endif

    Result ServerManagerBase::ProcessForSession(os::MultiWaitHolderType *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder)) == UserDataTag::Session);

        ServerSession *session = static_cast<ServerSession *>(holder);

        cmif::PointerAndSize tls_message(hipc::GetMessageBufferOnTls(), hipc::TlsMessageBufferSize);
        if (this->CanDeferInvokeRequest()) {
            const cmif::PointerAndSize &saved_message = session->m_saved_message;
            AMS_ABORT_UNLESS(tls_message.GetSize() == saved_message.GetSize());

            if (!session->m_has_received) {
                R_TRY(this->ReceiveRequest(session, tls_message));
                session->m_has_received = true;
                std::memcpy(saved_message.GetPointer(), tls_message.GetPointer(), tls_message.GetSize());
            } else {
                /* We were deferred and are re-receiving, so just memcpy. */
                std::memcpy(tls_message.GetPointer(), saved_message.GetPointer(), tls_message.GetSize());
            }

            /* Treat a meta "Context Invalidated" message as a success. */
            R_TRY_CATCH(this->ProcessRequest(session, tls_message)) {
                R_CONVERT(sf::impl::ResultRequestInvalidated, ResultSuccess());
            } R_END_TRY_CATCH;
        } else {
            if (!session->m_has_received) {
                R_TRY(this->ReceiveRequest(session, tls_message));
                session->m_has_received = true;

                #if AMS_SF_MITM_SUPPORTED
                if (this->CanManageMitmServers()) {
                    const cmif::PointerAndSize &saved_message = session->m_saved_message;
                    AMS_ABORT_UNLESS(tls_message.GetSize() == saved_message.GetSize());

                    std::memcpy(saved_message.GetPointer(), tls_message.GetPointer(), tls_message.GetSize());
                }
                #endif
            }

            R_TRY_CATCH(this->ProcessRequest(session, tls_message)) {
                R_CATCH(sf::ResultRequestDeferred)          { AMS_ABORT("Request Deferred on server which does not support deferral"); }
                R_CATCH(sf::impl::ResultRequestInvalidated) { AMS_ABORT("Request Invalidated on server which does not support deferral"); }
            } R_END_TRY_CATCH;
        }

        R_SUCCEED();
    }

    Result ServerManagerBase::Process(os::MultiWaitHolderType *holder) {
        switch (static_cast<UserDataTag>(os::GetMultiWaitHolderUserData(holder))) {
            case UserDataTag::Server:
                R_RETURN(this->ProcessForServer(holder));
            case UserDataTag::Session:
                R_RETURN(this->ProcessForSession(holder));
            #if AMS_SF_MITM_SUPPORTED
            case UserDataTag::MitmServer:
                AMS_ABORT_UNLESS(this->CanManageMitmServers());
                R_RETURN(this->ProcessForMitmServer(holder));
            #endif
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
