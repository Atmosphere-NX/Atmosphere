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
#include <stratosphere/sf/sf_mitm_config.hpp>
#include <stratosphere/sf/sf_service_object.hpp>
#include <stratosphere/sf/cmif/sf_cmif_pointer_and_size.hpp>
#include <stratosphere/sf/cmif/sf_cmif_service_object_holder.hpp>
#include <stratosphere/sf/hipc/sf_hipc_api.hpp>

namespace ams::sf::cmif {

    struct ServiceDispatchContext;

}

namespace ams::sf::hipc {

    class ServerSessionManager;
    class ServerManagerBase;

    namespace impl {

        class HipcManagerImpl;

    }

    class ServerSession : public os::MultiWaitHolderType {
        friend class ServerSessionManager;
        friend class ServerManagerBase;
        friend class impl::HipcManagerImpl;
        NON_COPYABLE(ServerSession);
        NON_MOVEABLE(ServerSession);
        private:
            cmif::ServiceObjectHolder m_srv_obj_holder;
            cmif::PointerAndSize m_pointer_buffer;
            cmif::PointerAndSize m_saved_message;
            #if AMS_SF_MITM_SUPPORTED
            util::TypedStorage<std::shared_ptr<::Service>> m_forward_service;
            #endif
            os::NativeHandle m_session_handle;
            bool m_is_closed;
            bool m_has_received;
            const bool m_has_forward_service;
        public:
            ServerSession(os::NativeHandle h, cmif::ServiceObjectHolder &&obj) : m_srv_obj_holder(std::move(obj)), m_session_handle(h), m_has_forward_service(false) {
                hipc::AttachMultiWaitHolderForReply(this, h);
                m_is_closed = false;
                m_has_received = false;
                #if AMS_SF_MITM_SUPPORTED
                AMS_ABORT_UNLESS(!this->IsMitmSession());
                #endif
            }

            ~ServerSession() {
                #if AMS_SF_MITM_SUPPORTED
                if (m_has_forward_service) {
                    util::DestroyAt(m_forward_service);
                }
                #endif
            }

            #if AMS_SF_MITM_SUPPORTED
            ServerSession(os::NativeHandle h, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) : m_srv_obj_holder(std::move(obj)), m_session_handle(h), m_has_forward_service(true) {
                hipc::AttachMultiWaitHolderForReply(this, h);
                m_is_closed = false;
                m_has_received = false;
                util::ConstructAt(m_forward_service, std::move(fsrv));
                AMS_ABORT_UNLESS(util::GetReference(m_forward_service) != nullptr);
            }

            ALWAYS_INLINE bool IsMitmSession() const {
                return m_has_forward_service;
            }

            Result ForwardRequest(const cmif::ServiceDispatchContext &ctx) const;

            static inline void ForwardServiceDeleter(Service *srv) {
                serviceClose(srv);
                delete srv;
            }

            static inline std::shared_ptr<::Service> CreateForwardService() {
                return std::shared_ptr<::Service>(new ::Service(), ForwardServiceDeleter);
            }
            #endif
    };

    class ServerSessionManager {
        private:
            template<typename Constructor>
            Result CreateSessionImpl(ServerSession **out, const Constructor &ctor) {
                /* Allocate session. */
                ServerSession *session_memory = this->AllocateSession();
                R_UNLESS(session_memory != nullptr, sf::hipc::ResultOutOfSessionMemory());
                ON_RESULT_FAILURE { this->DestroySession(session_memory); };

                /* Register session. */
                R_TRY(ctor(session_memory));

                /* Save new session to output. */
                *out = session_memory;
                R_SUCCEED();
            }
            void DestroySession(ServerSession *session);

            Result ProcessRequestImpl(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message);
            virtual void RegisterServerSessionToWait(ServerSession *session) = 0;
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
            Result RegisterSessionImpl(ServerSession *session_memory, os::NativeHandle session_handle, cmif::ServiceObjectHolder &&obj);
            Result AcceptSessionImpl(ServerSession *session_memory, os::NativeHandle port_handle, cmif::ServiceObjectHolder &&obj);

            #if AMS_SF_MITM_SUPPORTED
            Result RegisterMitmSessionImpl(ServerSession *session_memory, os::NativeHandle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            Result AcceptMitmSessionImpl(ServerSession *session_memory, os::NativeHandle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            #endif

            Result ReceiveRequest(ServerSession *session, const cmif::PointerAndSize &message) {
                R_RETURN(this->ReceiveRequestImpl(session, message));
            }

            Result RegisterSession(ServerSession **out, os::NativeHandle session_handle, cmif::ServiceObjectHolder &&obj) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    R_RETURN(this->RegisterSessionImpl(session_memory, session_handle, std::forward<cmif::ServiceObjectHolder>(obj)));
                };
                R_RETURN(this->CreateSessionImpl(out, ctor));
            }

            Result AcceptSession(ServerSession **out, os::NativeHandle port_handle, cmif::ServiceObjectHolder &&obj) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    R_RETURN(this->AcceptSessionImpl(session_memory, port_handle, std::forward<cmif::ServiceObjectHolder>(obj)));
                };
                R_RETURN(this->CreateSessionImpl(out, ctor));
            }

            #if AMS_SF_MITM_SUPPORTED
            Result RegisterMitmSession(ServerSession **out, os::NativeHandle mitm_session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    R_RETURN(this->RegisterMitmSessionImpl(session_memory, mitm_session_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv)));
                };
                R_RETURN(this->CreateSessionImpl(out, ctor));
            }

            Result AcceptMitmSession(ServerSession **out, os::NativeHandle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv) {
                auto ctor = [&](ServerSession *session_memory) -> Result {
                    R_RETURN(this->AcceptMitmSessionImpl(session_memory, mitm_port_handle, std::forward<cmif::ServiceObjectHolder>(obj), std::forward<std::shared_ptr<::Service>>(fsrv)));
                };
                R_RETURN(this->CreateSessionImpl(out, ctor));
            }
            #endif
        public:
            Result RegisterSession(os::NativeHandle session_handle, cmif::ServiceObjectHolder &&obj);
            Result AcceptSession(os::NativeHandle port_handle, cmif::ServiceObjectHolder &&obj);

            #if AMS_SF_MITM_SUPPORTED
            Result RegisterMitmSession(os::NativeHandle session_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            Result AcceptMitmSession(os::NativeHandle mitm_port_handle, cmif::ServiceObjectHolder &&obj, std::shared_ptr<::Service> &&fsrv);
            #endif

            template<typename Interface>
            Result AcceptSession(os::NativeHandle port_handle, SharedPointer<Interface> obj) {
                R_RETURN(this->AcceptSession(port_handle, cmif::ServiceObjectHolder(std::move(obj))));
            }

            #if AMS_SF_MITM_SUPPORTED
            template<typename Interface>
            Result AcceptMitmSession(os::NativeHandle mitm_port_handle, SharedPointer<Interface> obj, std::shared_ptr<::Service> &&fsrv) {
                R_RETURN(this->AcceptMitmSession(mitm_port_handle, cmif::ServiceObjectHolder(std::move(obj)), std::forward<std::shared_ptr<::Service>>(fsrv)));
            }
            #endif

            Result ProcessRequest(ServerSession *session, const cmif::PointerAndSize &message);

            virtual ServerSessionManager *GetSessionManagerByTag(u32 tag) {
                /* This is unused. */
                AMS_UNUSED(tag);
                return this;
            }
    };

}
