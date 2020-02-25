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
#include "sf_hipc_mitm_query_api.hpp"

namespace ams::sf::hipc {

    ServerManagerBase::ServerBase::~ServerBase() { /* Pure virtual destructor, to prevent linker errors. */ }

    Result ServerManagerBase::InstallMitmServerImpl(Handle *out_port_handle, sm::ServiceName service_name, ServerManagerBase::MitmQueryFunction query_func) {
        /* Install the Mitm. */
        Handle query_handle;
        R_TRY(sm::mitm::InstallMitm(out_port_handle, &query_handle, service_name));

        /* Register the query handle. */
        impl::RegisterMitmQueryHandle(query_handle, query_func);
        return ResultSuccess();
    }

    void ServerManagerBase::RegisterSessionToWaitList(ServerSession *session) {
        session->has_received = false;

        /* Set user data tag. */
        session->SetUserData(static_cast<uintptr_t>(UserDataTag::Session));

        this->RegisterToWaitList(session);
    }

    void ServerManagerBase::RegisterToWaitList(os::WaitableHolder *holder) {
        std::scoped_lock lk(this->waitlist_mutex);
        this->waitlist.LinkWaitableHolder(holder);
        this->notify_event.Signal();
    }

    void ServerManagerBase::ProcessWaitList() {
        std::scoped_lock lk(this->waitlist_mutex);
        this->waitable_manager.MoveAllFrom(&this->waitlist);
    }

    os::WaitableHolder *ServerManagerBase::WaitSignaled() {
        std::scoped_lock lk(this->waitable_selection_mutex);
        while (true) {
            this->ProcessWaitList();
            auto selected = this->waitable_manager.WaitAny();
            if (selected == &this->request_stop_event_holder) {
                return nullptr;
            } else if (selected == &this->notify_event_holder) {
                this->notify_event.Reset();
            } else {
                selected->UnlinkFromWaitableManager();
                return selected;
            }
        }
    }

    void ServerManagerBase::ResumeProcessing() {
        this->request_stop_event.Reset();
    }

    void ServerManagerBase::RequestStopProcessing() {
        this->request_stop_event.Signal();
    }

    void ServerManagerBase::AddUserWaitableHolder(os::WaitableHolder *waitable) {
        const auto user_data_tag = static_cast<UserDataTag>(waitable->GetUserData());
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Server);
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::MitmServer);
        AMS_ABORT_UNLESS(user_data_tag != UserDataTag::Session);
        this->RegisterToWaitList(waitable);
    }

    Result ServerManagerBase::ProcessForServer(os::WaitableHolder *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(holder->GetUserData()) == UserDataTag::Server);

        ServerBase *server = static_cast<ServerBase *>(holder);
        ON_SCOPE_EXIT { this->RegisterToWaitList(server); };

        /* Create resources for new session. */
        cmif::ServiceObjectHolder obj;
        std::shared_ptr<::Service> fsrv;
        server->CreateSessionObjectHolder(&obj, &fsrv);

        /* Not a mitm server, so we must have no forward service. */
        AMS_ABORT_UNLESS(fsrv == nullptr);

        /* Try to accept. */
        return this->AcceptSession(server->port_handle, std::move(obj));
    }

    Result ServerManagerBase::ProcessForMitmServer(os::WaitableHolder *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(holder->GetUserData()) == UserDataTag::MitmServer);

        ServerBase *server = static_cast<ServerBase *>(holder);
        ON_SCOPE_EXIT { this->RegisterToWaitList(server); };

        /* Create resources for new session. */
        cmif::ServiceObjectHolder obj;
        std::shared_ptr<::Service> fsrv;
        server->CreateSessionObjectHolder(&obj, &fsrv);

        /* Mitm server, so we must have forward service. */
        AMS_ABORT_UNLESS(fsrv != nullptr);

        /* Try to accept. */
        return this->AcceptMitmSession(server->port_handle, std::move(obj), std::move(fsrv));
    }

    Result ServerManagerBase::ProcessForSession(os::WaitableHolder *holder) {
        AMS_ABORT_UNLESS(static_cast<UserDataTag>(holder->GetUserData()) == UserDataTag::Session);

        ServerSession *session = static_cast<ServerSession *>(holder);

        cmif::PointerAndSize tls_message(armGetTls(), hipc::TlsMessageBufferSize);
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

    void ServerManagerBase::ProcessDeferredSessions() {
        /* Iterate over the list of deferred sessions, and see if we can't do anything. */
        std::scoped_lock lk(this->deferred_session_mutex);

        /* Undeferring a request may undefer another request. We'll continue looping until everything is stable. */
        bool needs_undefer_all = true;
        while (needs_undefer_all) {
            needs_undefer_all = false;

            auto it = this->deferred_session_list.begin();
            while (it != this->deferred_session_list.end()) {
                ServerSession *session = static_cast<ServerSession *>(&*it);
                R_TRY_CATCH(this->ProcessForSession(session)) {
                    R_CATCH(sf::ResultRequestDeferred) {
                        /* Session is still deferred, so let's continue. */
                        it++;
                        continue;
                    }
                    R_CATCH(sf::impl::ResultRequestInvalidated) {
                        /* Session is no longer deferred! */
                        it = this->deferred_session_list.erase(it);
                        needs_undefer_all = true;
                        continue;
                    }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                /* We succeeded! Remove from deferred list. */
                it = this->deferred_session_list.erase(it);
                needs_undefer_all = true;
            }
        }
    }

    Result ServerManagerBase::Process(os::WaitableHolder *holder) {
        switch (static_cast<UserDataTag>(holder->GetUserData())) {
            case UserDataTag::Server:
                return this->ProcessForServer(holder);
                break;
            case UserDataTag::MitmServer:
                return this->ProcessForMitmServer(holder);
                break;
            case UserDataTag::Session:
                /* Try to process for session. */
                R_TRY_CATCH(this->ProcessForSession(holder)) {
                    R_CATCH(sf::ResultRequestDeferred) {
                        /* The session was deferred, so push it onto the deferred session list. */
                        std::scoped_lock lk(this->deferred_session_mutex);
                        this->deferred_session_list.push_back(*static_cast<ServerSession *>(holder));
                        return ResultSuccess();
                    }
                } R_END_TRY_CATCH;

                /* We successfully invoked a command...so let's see if anything can be undeferred. */
                this->ProcessDeferredSessions();
                return ResultSuccess();
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    bool ServerManagerBase::WaitAndProcessImpl() {
        auto waitable = this->WaitSignaled();
        if (!waitable) {
            return false;
        }
        R_ABORT_UNLESS(this->Process(waitable));
        return true;
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
