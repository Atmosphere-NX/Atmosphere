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
#include <stratosphere/sf/hipc/sf_hipc_server_domain_session_manager.hpp>
#include <stratosphere/sm.hpp>

namespace ams::sf::hipc {

    struct DefaultServerManagerOptions {
        static constexpr size_t PointerBufferSize = 0;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
        static constexpr bool CanDeferInvokeRequest = false;
        static constexpr bool CanManageMitmServers  = false;
    };

    static constexpr size_t ServerSessionCountMax = 0x40;
    static_assert(ServerSessionCountMax == 0x40, "ServerSessionCountMax isn't 0x40 somehow, this assert is a reminder that this will break lots of things");

    template<size_t, typename, size_t>
    class ServerManager;

    class ServerManagerBase : public ServerDomainSessionManager {
        NON_COPYABLE(ServerManagerBase);
        NON_MOVEABLE(ServerManagerBase);
        public:
            using MitmQueryFunction = bool (*)(const sm::MitmProcessInfo &);
        private:
            enum class UserDataTag : uintptr_t {
                Server      = 1,
                Session     = 2,
                MitmServer  = 3,
            };
        protected:
            using ServerDomainSessionManager::DomainEntryStorage;
            using ServerDomainSessionManager::DomainStorage;
        protected:
            class Server : public os::MultiWaitHolderType {
                friend class ServerManagerBase;
                template<size_t, typename, size_t>
                friend class ServerManager;
                NON_COPYABLE(Server);
                NON_MOVEABLE(Server);
                private:
                    cmif::ServiceObjectHolder m_static_object;
                    os::NativeHandle m_port_handle;
                    sm::ServiceName m_service_name;
                    int m_index;
                    bool m_service_managed;
                    bool m_is_mitm_server;
                public:
                    void AcknowledgeMitmSession(std::shared_ptr<::Service> *out_fsrv, sm::MitmProcessInfo *out_client_info) {
                        /* Check mitm server. */
                        AMS_ABORT_UNLESS(m_is_mitm_server);

                        /* Create forward service. */
                        *out_fsrv = ServerSession::CreateForwardService();

                        /* Get client info. */
                        R_ABORT_UNLESS(sm::mitm::AcknowledgeSession(out_fsrv->get(), out_client_info, m_service_name));
                    }
            };
        protected:
            static constinit inline bool g_is_any_deferred_supported = false;
            static constinit inline bool g_is_any_mitm_supported = false;
        private:
            /* Multiple wait management. */
            os::MultiWaitType m_multi_wait;
            os::Event m_request_stop_event;
            os::MultiWaitHolderType m_request_stop_event_holder;
            os::Event m_notify_event;
            os::MultiWaitHolderType m_notify_event_holder;

            os::SdkMutex m_selection_mutex;

            os::SdkMutex m_deferred_list_mutex;
            os::MultiWaitType m_deferred_list;

            /* Boolean values. */
            const bool m_is_defer_supported;
            const bool m_is_mitm_supported;
        private:
            virtual void RegisterServerSessionToWait(ServerSession *session) override final;
            void LinkToDeferredList(os::MultiWaitHolderType *holder);
            void LinkDeferred();

            bool WaitAndProcessImpl();

            Result ProcessForServer(os::MultiWaitHolderType *holder);
            Result ProcessForMitmServer(os::MultiWaitHolderType *holder);
            Result ProcessForSession(os::MultiWaitHolderType *holder);

            void RegisterServerImpl(Server *server, os::NativeHandle port_handle, bool is_mitm_server) {
                server->m_port_handle = port_handle;
                hipc::AttachMultiWaitHolderForAccept(server, port_handle);

                server->m_is_mitm_server = is_mitm_server;
                if (is_mitm_server) {
                    /* Mitm server. */
                    AMS_ABORT_UNLESS(this->CanManageMitmServers());
                    os::SetMultiWaitHolderUserData(server, static_cast<uintptr_t>(UserDataTag::MitmServer));
                } else {
                    /* Non-mitm server. */
                    os::SetMultiWaitHolderUserData(server, static_cast<uintptr_t>(UserDataTag::Server));
                }

                os::LinkMultiWaitHolder(std::addressof(m_multi_wait), server);
            }

            void RegisterServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, os::NativeHandle port_handle, bool is_mitm_server) {
                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->m_service_managed = false;

                if (static_holder) {
                    server->m_static_object = std::move(static_holder);
                } else {
                    server->m_index = index;
                }

                this->RegisterServerImpl(server, port_handle, is_mitm_server);
            }

            Result RegisterServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, sm::ServiceName service_name, size_t max_sessions) {
                /* Register service. */
                os::NativeHandle port_handle;
                R_TRY(sm::RegisterService(&port_handle, service_name, max_sessions, false));

                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->m_service_managed = true;
                server->m_service_name = service_name;

                if (static_holder) {
                    server->m_static_object = std::move(static_holder);
                } else {
                    server->m_index = index;
                }

                this->RegisterServerImpl(server, port_handle, false);

                return ResultSuccess();
            }

            Result InstallMitmServerImpl(os::NativeHandle *out_port_handle, sm::ServiceName service_name, MitmQueryFunction query_func);
        protected:
            virtual Server *AllocateServer() = 0;
            virtual void DestroyServer(Server *server)  = 0;
            virtual Result OnNeedsToAccept(int port_index, Server *server) {
                AMS_UNUSED(port_index, server);
                AMS_ABORT("OnNeedsToAccept must be overridden when using indexed ports");
            }

            template<typename Interface>
            Result AcceptImpl(Server *server, SharedPointer<Interface> p) {
                return ServerSessionManager::AcceptSession(server->m_port_handle, std::move(p));
            }

            template<typename Interface>
            Result AcceptMitmImpl(Server *server, SharedPointer<Interface> p, std::shared_ptr<::Service> forward_service) {
                AMS_ABORT_UNLESS(this->CanManageMitmServers());
                return ServerSessionManager::AcceptMitmSession(server->m_port_handle, std::move(p), std::move(forward_service));
            }

            template<typename Interface>
            Result RegisterMitmServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, sm::ServiceName service_name) {
                /* Install mitm service. */
                os::NativeHandle port_handle;
                R_TRY(this->InstallMitmServerImpl(&port_handle, service_name, &Interface::ShouldMitm));

                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->m_service_managed = true;
                server->m_service_name = service_name;

                if (static_holder) {
                    server->m_static_object = std::move(static_holder);
                } else {
                    server->m_index = index;
                }

                this->RegisterServerImpl(server, port_handle, true);

                return ResultSuccess();
            }
        public:
            ServerManagerBase(DomainEntryStorage *entry_storage, size_t entry_count, bool defer_supported, bool mitm_supported) :
                ServerDomainSessionManager(entry_storage, entry_count),
                m_request_stop_event(os::EventClearMode_ManualClear), m_notify_event(os::EventClearMode_ManualClear),
                m_selection_mutex(), m_deferred_list_mutex(), m_is_defer_supported(defer_supported), m_is_mitm_supported(mitm_supported)
            {
                /* Link multi-wait holders. */
                os::InitializeMultiWait(std::addressof(m_multi_wait));
                os::InitializeMultiWaitHolder(std::addressof(m_request_stop_event_holder), m_request_stop_event.GetBase());
                os::LinkMultiWaitHolder(std::addressof(m_multi_wait), std::addressof(m_request_stop_event_holder));
                os::InitializeMultiWaitHolder(std::addressof(m_notify_event_holder), m_notify_event.GetBase());
                os::LinkMultiWaitHolder(std::addressof(m_multi_wait), std::addressof(m_notify_event_holder));

                os::InitializeMultiWait(std::addressof(m_deferred_list));
            }

            static ALWAYS_INLINE bool CanAnyDeferInvokeRequest() {
                return g_is_any_deferred_supported;
            }

            static ALWAYS_INLINE bool CanAnyManageMitmServers() {
                return g_is_any_mitm_supported;
            }

            ALWAYS_INLINE bool CanDeferInvokeRequest() const {
                return CanAnyDeferInvokeRequest() && m_is_defer_supported;
            }

            ALWAYS_INLINE bool CanManageMitmServers() const {
                return CanAnyManageMitmServers() && m_is_mitm_supported;
            }

            template<typename Interface>
            void RegisterObjectForServer(SharedPointer<Interface> static_object, os::NativeHandle port_handle) {
                this->RegisterServerImpl(0, cmif::ServiceObjectHolder(std::move(static_object)), port_handle, false);
            }

            template<typename Interface>
            Result RegisterObjectForServer(SharedPointer<Interface> static_object, sm::ServiceName service_name, size_t max_sessions) {
                return this->RegisterServerImpl(0, cmif::ServiceObjectHolder(std::move(static_object)), service_name, max_sessions);
            }

            void RegisterServer(int port_index, os::NativeHandle port_handle) {
                this->RegisterServerImpl(port_index, cmif::ServiceObjectHolder(), port_handle, false);
            }

            Result RegisterServer(int port_index, sm::ServiceName service_name, size_t max_sessions) {
                return this->RegisterServerImpl(port_index, cmif::ServiceObjectHolder(), service_name, max_sessions);
            }

            /* Processing. */
            os::MultiWaitHolderType *WaitSignaled();

            void   ResumeProcessing();
            void   RequestStopProcessing();
            void   AddUserMultiWaitHolder(os::MultiWaitHolderType *holder);

            Result Process(os::MultiWaitHolderType *holder);
            void   WaitAndProcess();
            void   LoopProcess();
    };

    template<size_t MaxServers, typename ManagerOptions = DefaultServerManagerOptions, size_t MaxSessions = ServerSessionCountMax - MaxServers>
    class ServerManager : public ServerManagerBase {
        NON_COPYABLE(ServerManager);
        NON_MOVEABLE(ServerManager);
        static_assert(MaxServers  <= ServerSessionCountMax, "MaxServers can never be larger than ServerSessionCountMax (0x40).");
        static_assert(MaxSessions <= ServerSessionCountMax, "MaxSessions can never be larger than ServerSessionCountMax (0x40).");
        static_assert(MaxServers + MaxSessions <= ServerSessionCountMax, "MaxServers + MaxSessions can never be larger than ServerSessionCountMax (0x40).");
        private:
            static constexpr inline bool DomainCountsValid = [] {
                if constexpr (ManagerOptions::MaxDomains > 0) {
                    return ManagerOptions::MaxDomainObjects > 0;
                } else {
                    return ManagerOptions::MaxDomainObjects == 0;
                }
            }();
            static_assert(DomainCountsValid, "Invalid Domain Counts");
        protected:
            using ServerManagerBase::DomainEntryStorage;
            using ServerManagerBase::DomainStorage;
        private:
            /* Resource storage. */
            os::SdkMutex m_resource_mutex;
            util::TypedStorage<Server> m_server_storages[MaxServers];
            bool m_server_allocated[MaxServers];
            util::TypedStorage<ServerSession> m_session_storages[MaxSessions];
            bool m_session_allocated[MaxSessions];
            u8 m_pointer_buffer_storage[0x10 + (MaxSessions * ManagerOptions::PointerBufferSize)];
            u8 m_saved_message_storage[0x10 + (MaxSessions * ((ManagerOptions::CanDeferInvokeRequest || ManagerOptions::CanManageMitmServers) ? hipc::TlsMessageBufferSize : 0))];
            uintptr_t m_pointer_buffers_start;
            uintptr_t m_saved_messages_start;

            /* Domain resources. */
            DomainStorage m_domain_storages[ManagerOptions::MaxDomains];
            bool m_domain_allocated[ManagerOptions::MaxDomains];
            DomainEntryStorage m_domain_entry_storages[ManagerOptions::MaxDomainObjects];
        private:
            constexpr inline size_t GetServerIndex(const Server *server) const {
                const size_t i = server - GetPointer(m_server_storages[0]);
                AMS_ABORT_UNLESS(i < MaxServers);
                return i;
            }

            constexpr inline size_t GetSessionIndex(const ServerSession *session) const {
                const size_t i = session - GetPointer(m_session_storages[0]);
                AMS_ABORT_UNLESS(i < MaxSessions);
                return i;
            }

            constexpr inline cmif::PointerAndSize GetObjectBySessionIndex(const ServerSession *session, uintptr_t start, size_t size) const {
                return cmif::PointerAndSize(start + this->GetSessionIndex(session) * size, size);
            }
        protected:
            virtual ServerSession *AllocateSession() override final {
                if constexpr (MaxSessions > 0) {
                    std::scoped_lock lk(m_resource_mutex);

                    for (size_t i = 0; i < MaxSessions; i++) {
                        if (!m_session_allocated[i]) {
                            m_session_allocated[i] = true;
                            return GetPointer(m_session_storages[i]);
                        }
                    }
                }

                return nullptr;
            }

            virtual void FreeSession(ServerSession *session) override final {
                std::scoped_lock lk(m_resource_mutex);
                const size_t index = this->GetSessionIndex(session);
                AMS_ABORT_UNLESS(m_session_allocated[index]);
                m_session_allocated[index] = false;
            }

            virtual Server *AllocateServer() override final {
                if constexpr (MaxServers > 0) {
                    std::scoped_lock lk(m_resource_mutex);

                    for (size_t i = 0; i < MaxServers; i++) {
                        if (!m_server_allocated[i]) {
                            m_server_allocated[i] = true;
                            return GetPointer(m_server_storages[i]);
                        }
                    }
                }

                return nullptr;
            }

            virtual void DestroyServer(Server *server) override final {
                std::scoped_lock lk(m_resource_mutex);
                const size_t index = this->GetServerIndex(server);
                AMS_ABORT_UNLESS(m_server_allocated[index]);
                {
                    os::UnlinkMultiWaitHolder(server);
                    os::FinalizeMultiWaitHolder(server);
                    if (server->m_service_managed) {
                        if constexpr (ManagerOptions::CanManageMitmServers) {
                            if (server->m_is_mitm_server) {
                                R_ABORT_UNLESS(sm::mitm::UninstallMitm(server->m_service_name));
                            } else {
                                R_ABORT_UNLESS(sm::UnregisterService(server->m_service_name));
                            }
                        } else {
                            R_ABORT_UNLESS(sm::UnregisterService(server->m_service_name));
                        }
                        os::CloseNativeHandle(server->m_port_handle);
                    }
                }
                m_server_allocated[index] = false;
            }

            virtual void *AllocateDomain() override final {
                std::scoped_lock lk(m_resource_mutex);
                for (size_t i = 0; i < ManagerOptions::MaxDomains; i++) {
                    if (!m_domain_allocated[i]) {
                        m_domain_allocated[i] = true;
                        return GetPointer(m_domain_storages[i]);
                    }
                }
                return nullptr;
            }

            virtual void FreeDomain(void *domain) override final {
                std::scoped_lock lk(m_resource_mutex);
                DomainStorage *ptr = static_cast<DomainStorage *>(domain);
                const size_t index = ptr - m_domain_storages;
                AMS_ABORT_UNLESS(index < ManagerOptions::MaxDomains);
                AMS_ABORT_UNLESS(m_domain_allocated[index]);
                m_domain_allocated[index] = false;
            }

            virtual cmif::PointerAndSize GetSessionPointerBuffer(const ServerSession *session) const override final {
                if constexpr (ManagerOptions::PointerBufferSize > 0) {
                    return this->GetObjectBySessionIndex(session, m_pointer_buffers_start, ManagerOptions::PointerBufferSize);
                } else {
                    return cmif::PointerAndSize();
                }
            }

            virtual cmif::PointerAndSize GetSessionSavedMessageBuffer(const ServerSession *session) const override final {
                if constexpr (ManagerOptions::CanDeferInvokeRequest || ManagerOptions::CanManageMitmServers) {
                    return this->GetObjectBySessionIndex(session, m_saved_messages_start, hipc::TlsMessageBufferSize);
                } else {
                    return cmif::PointerAndSize();
                }
            }
        public:
            ServerManager() : ServerManagerBase(m_domain_entry_storages, ManagerOptions::MaxDomainObjects, ManagerOptions::CanDeferInvokeRequest, ManagerOptions::CanManageMitmServers), m_resource_mutex() {
                /* Clear storages. */
                #define SF_SM_MEMCLEAR(obj) if constexpr (sizeof(obj) > 0) { std::memset(obj, 0, sizeof(obj)); }
                SF_SM_MEMCLEAR(m_server_storages);
                SF_SM_MEMCLEAR(m_server_allocated);
                SF_SM_MEMCLEAR(m_session_storages);
                SF_SM_MEMCLEAR(m_session_allocated);
                SF_SM_MEMCLEAR(m_pointer_buffer_storage);
                SF_SM_MEMCLEAR(m_saved_message_storage);
                SF_SM_MEMCLEAR(m_domain_allocated);
                #undef SF_SM_MEMCLEAR

                /* Set resource starts. */
                m_pointer_buffers_start = util::AlignUp(reinterpret_cast<uintptr_t>(m_pointer_buffer_storage), 0x10);
                m_saved_messages_start  = util::AlignUp(reinterpret_cast<uintptr_t>(m_saved_message_storage),  0x10);

                /* Update globals. */
                if constexpr (ManagerOptions::CanDeferInvokeRequest) {
                    ServerManagerBase::g_is_any_deferred_supported = true;
                }
                if constexpr (ManagerOptions::CanManageMitmServers) {
                    ServerManagerBase::g_is_any_mitm_supported = true;
                }
            }

            ~ServerManager() {
                /* Close all sessions. */
                if constexpr (MaxSessions > 0) {
                    for (size_t i = 0; i < MaxSessions; i++) {
                        if (m_session_allocated[i]) {
                            this->CloseSessionImpl(GetPointer(m_session_storages[i]));
                        }
                    }
                }

                /* Close all servers. */
                if constexpr (MaxServers > 0) {
                    for (size_t i = 0; i < MaxServers; i++) {
                        if (m_server_allocated[i]) {
                            this->DestroyServer(GetPointer(m_server_storages[i]));
                        }
                    }
                }
            }
        public:
            template<typename Interface, bool Enable = ManagerOptions::CanManageMitmServers, typename = typename std::enable_if<Enable>::type>
            Result RegisterMitmServer(int port_index, sm::ServiceName service_name) {
                AMS_ABORT_UNLESS(this->CanManageMitmServers());
                return this->template RegisterMitmServerImpl<Interface>(port_index, cmif::ServiceObjectHolder(), service_name);
            }
    };

}
