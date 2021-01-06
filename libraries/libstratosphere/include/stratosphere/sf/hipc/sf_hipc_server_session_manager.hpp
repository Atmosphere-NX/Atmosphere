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

#pragma once
#include "../sf_common.hpp"
#include "../sf_service_object.hpp"
#include "../cmif/sf_cmif_pointer_and_size.hpp"
#include "../cmif/sf_cmif_service_object_holder.hpp"
#include "sf_hipc_api.hpp"

namespace ams::sf::cmif {

    struct ServiceDispatchContext;

}

namespace ams::sf::hipc {

    class ServerSessionManager;
    class ServerManagerBase;

    namespace impl {

        class HipcManager;

    }

    class ServerSession : public os::WaitableHolderType {
        friend class ServerSessionManager;
        friend class ServerManagerBase;
        friend class impl::HipcManager;
        NON_COPYABLE(ServerSession);
        NON_MOVEABLE(ServerSession);
        private:
            cmif::ServiceObjectHolder srv_obj_holder;
            cmif::PointerAndSize pointer_buffer;
            cmif::PointerAndSize saved_message;
            std::shared_ptr<::Service> forward_service;
            Handle session_handle;
            bool is_closed;
            bool has_received;
        public:
            ServerSession(Handle h, cmif::ServiceObjectHolder &&obj) : srv_obj_holder(std::move(obj)), session_handle(h) {
                hipc::AttachWaitableHolderForReply(this, h);
                this->is_closed = false;
                this->has_received = false;
                this->forward_service = nullptr;
                AMS_ABORT_UNLESS(!this->IsMitmSession());
            }

            ServerSession(Handle h, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) : srv_obj_holder(std::move(obj)), session_handle(h) {
                hipc::AttachWaitableHolderForReply(this, h);
                this->is_closed = false;
                this->has_received = false;
                this->forward_service = std::move(fsrv);
                AMS_ABORT_UNLESS(this->IsMitmSession());
            }

            bool IsMitmSession() const {
                return this->forward_service != nullptr;
            }

            Result ForwardRequest(const cmif::ServiceDispatchContext &ctx) const;

            static inline void ForwardServiceDeleter(Service *srv) {
                serviceClose(srv);
                delete srv;
            }

            static inline std::shared_ptr<::Service> CreateForwardService() {
                return std::shared_ptr<::Service>(new ::Service(), ForwardServiceDeleter);
            }
    };

    class ServerSessionManager {
        private:
            template<typename Constructor>
            Result CreateSessionImpl(ServerSession **out, const Constructor &ctor) {
                /* Allocate session. */
                ServerSession *session_memory = this->AllocateSession();
                R_UNLESS(session_memory != nullptr, sf::hipc::ResultOutOfSessionMemory());
                /* Register session. */
                bool succeeded = false;
                ON_SCOPE_EXIT {
                    if (!succeeded) {
                        this->DestroySession(session_memory);
                    }
                };
                R_TRY(ctor(session_memory));
                /* Save new session to output. */
                succeeded = true;
                *out = session_memory;
                return ResultSuccess();
            }
            void DestroySession(ServerSession *session);

            Result ProcessRequestImpl(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message);
            virtual void RegisterSessionToWaitList(ServerSession *session) = 0;
        protected:
            Result DispatchRequest(cmif::ServiceObjectHolder &&obj, ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message);
            virtual Result DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message);
        protected:
            virtual ServerSession *AllocateSession() = 0;
            virtual void FreeSession(ServerSession *session) = 0;
            virtual cmif::PointerAndSize GetSessionPointerBuffer(const ServerSession *session) const = 0;
            virtual cmif::PointerAndSize GetSessionSavedMessageBuffer(const ServerSession *session) const = 0;

            Result ReceiveRequestImpl(ServerSession *session, const cmif::PointerAndSize &message);
            void   CloseSessionImpl(ServerSession *session);
            Result RegisterSessionImpl(ServerSession *session_memory, Handle session_handle, cmif::ServiceObjectHolder &&obj);
            Result AcceptSessionImpl(ServerSession *session_memory, Handle port_handle, cmif::ServiceObjectHolder &&obj);
            Result RegisterMitmSessionImpl(ServerSession *session_memory, Handle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            Result AcceptMitmSessionImpl(ServerSession *session_memory, Handle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);

            Result ReceiveRequest(ServerSession *session, const cmif::PointerAndSize &message) {
                return this->ReceiveRequestImpl(session, message);
            }

            Result RegisterSession(ServerSession **out, Handle session_handle, cmif::ServiceObjectHolder &&obj) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    return this->RegisterSessionImpl(session_memory, session_handle, std::forward<cmif::ServiceObjectHolder>(obj));
                };
                return this->CreateSessionImpl(out, ctor);
            }

            Result AcceptSession(ServerSession **out, Handle port_handle, cmif::ServiceObjectHolder &&obj) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    return this->AcceptSessionImpl(session_memory, port_handle, std::forward<cmif::ServiceObjectHolder>(obj));
                };
                return this->CreateSessionImpl(out, ctor);
            }

            Result RegisterMitmSession(ServerSession **out, Handle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    return this->RegisterMitmSessionImpl(session_memory, mitm_session_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv));
                };
                return this->CreateSessionImpl(out, ctor);
            }

            Result AcceptMitmSession(ServerSession **out, Handle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    return this->AcceptMitmSessionImpl(session_memory, mitm_port_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv));
                };
                return this->CreateSessionImpl(out, ctor);
            }
        public:
            Result RegisterSession(Handle session_handle, cmif::ServiceObjectHolder &&obj);
            Result AcceptSession(Handle port_handle, cmif::ServiceObjectHolder &&obj);
            Result RegisterMitmSession(Handle session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            Result AcceptMitmSession(Handle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);

            template<typename ServiceImpl>
            Result AcceptSession(Handle port_handle, std::shared_ptr<ServiceImpl> obj) {
                return this->AcceptSession(port_handle, cmif::ServiceObjectHolder(std::move(obj)));
            }

            template<typename ServiceImpl>
            Result AcceptMitmSession(Handle mitm_port_handle, std::shared_ptr<ServiceImpl> obj, std::shared_ptr<::Service> &&fsrv) {
                return this->AcceptMitmSession(mitm_port_handle, cmif::ServiceObjectHolder(std::move(obj)), std::forward<std::shared_ptr<::Service>>(fsrv));
            }

            Result ProcessRequest(ServerSession *session, const cmif::PointerAndSize &message);

            virtual ServerSessionManager *GetSessionManagerByTag(u32 tag) {
                /* This is unused. */
                return this;
            }
    };

}
