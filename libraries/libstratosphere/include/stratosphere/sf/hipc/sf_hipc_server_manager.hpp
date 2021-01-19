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
#include "sf_hipc_server_domain_session_manager.hpp"
#include "../../sm.hpp"

namespace ams::sf::hipc {

    struct DefaultServerManagerOptions {
        static constexpr size_t PointerBufferSize = 0;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
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
            class Server : public os::WaitableHolderType {
                friend class ServerManagerBase;
                template<size_t, typename, size_t>
                friend class ServerManager;
                NON_COPYABLE(Server);
                NON_MOVEABLE(Server);
                private:
                    cmif::ServiceObjectHolder static_object;
                    ::Handle port_handle;
                    sm::ServiceName service_name;
                    int index;
                    bool service_managed;
                    bool is_mitm_server;
                public:
                    void AcknowledgeMitmSession(std::shared_ptr<::Service> *out_fsrv, sm::MitmProcessInfo *out_client_info) {
                        /* Check mitm server. */
                        AMS_ABORT_UNLESS(this->is_mitm_server);

                        /* Create forward service. */
                        *out_fsrv = ServerSession::CreateForwardService();

                        /* Get client info. */
                        R_ABORT_UNLESS(sm::mitm::AcknowledgeSession(out_fsrv->get(), out_client_info, this->service_name));
                    }
            };
        private:
            /* Management of waitables. */
            os::WaitableManagerType waitable_manager;
            os::Event request_stop_event;
            os::WaitableHolderType request_stop_event_holder;
            os::Event notify_event;
            os::WaitableHolderType notify_event_holder;

            os::Mutex waitable_selection_mutex;

            os::Mutex waitlist_mutex;
            os::WaitableManagerType waitlist;
        private:
            virtual void RegisterSessionToWaitList(ServerSession *session) override final;
            void RegisterToWaitList(os::WaitableHolderType *holder);
            void ProcessWaitList();

            bool WaitAndProcessImpl();

            Result ProcessForServer(os::WaitableHolderType *holder);
            Result ProcessForMitmServer(os::WaitableHolderType *holder);
            Result ProcessForSession(os::WaitableHolderType *holder);

            void RegisterServerImpl(Server *server, Handle port_handle, bool is_mitm_server) {
                server->port_handle = port_handle;
                hipc::AttachWaitableHolderForAccept(server, port_handle);

                server->is_mitm_server = is_mitm_server;
                if (is_mitm_server) {
                    /* Mitm server. */
                    os::SetWaitableHolderUserData(server, static_cast<uintptr_t>(UserDataTag::MitmServer));
                } else {
                    /* Non-mitm server. */
                    os::SetWaitableHolderUserData(server, static_cast<uintptr_t>(UserDataTag::Server));
                }

                os::LinkWaitableHolder(std::addressof(this->waitable_manager), server);
            }

            void RegisterServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, Handle port_handle, bool is_mitm_server) {
                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->service_managed = false;

                if (static_holder) {
                    server->static_object = std::move(static_holder);
                } else {
                    server->index = index;
                }

                this->RegisterServerImpl(server, port_handle, is_mitm_server);
            }

            Result RegisterServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, sm::ServiceName service_name, size_t max_sessions) {
                /* Register service. */
                Handle port_handle;
                R_TRY(sm::RegisterService(&port_handle, service_name, max_sessions, false));

                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->service_managed = true;
                server->service_name = service_name;

                if (static_holder) {
                    server->static_object = std::move(static_holder);
                } else {
                    server->index = index;
                }

                this->RegisterServerImpl(server, port_handle, false);

                return ResultSuccess();
            }

            template<typename Interface>
            Result RegisterMitmServerImpl(int index, cmif::ServiceObjectHolder &&static_holder, sm::ServiceName service_name) {
                /* Install mitm service. */
                Handle port_handle;
                R_TRY(this->InstallMitmServerImpl(&port_handle, service_name, &Interface::ShouldMitm));

                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                server->service_managed = true;
                server->service_name = service_name;

                if (static_holder) {
                    server->static_object = std::move(static_holder);
                } else {
                    server->index = index;
                }

                this->RegisterServerImpl(server, port_handle, true);

                return ResultSuccess();
            }

            Result InstallMitmServerImpl(Handle *out_port_handle, sm::ServiceName service_name, MitmQueryFunction query_func);
        protected:
            virtual Server *AllocateServer() = 0;
            virtual void DestroyServer(Server *server)  = 0;
            virtual Result OnNeedsToAccept(int port_index, Server *server) {
                AMS_ABORT("OnNeedsToAccept must be overridden when using indexed ports");
            }

            template<typename Interface>
            Result AcceptImpl(Server *server, SharedPointer<Interface> p) {
                return ServerSessionManager::AcceptSession(server->port_handle, std::move(p));
            }

            template<typename Interface>
            Result AcceptMitmImpl(Server *server, SharedPointer<Interface> p, std::shared_ptr<::Service> forward_service) {
                return ServerSessionManager::AcceptMitmSession(server->port_handle, std::move(p), std::move(forward_service));
            }
        public:
            ServerManagerBase(DomainEntryStorage *entry_storage, size_t entry_count) :
                ServerDomainSessionManager(entry_storage, entry_count),
                request_stop_event(os::EventClearMode_ManualClear), notify_event(os::EventClearMode_ManualClear),
                waitable_selection_mutex(false), waitlist_mutex(false)
            {
                /* Link waitables. */
                os::InitializeWaitableManager(std::addressof(this->waitable_manager));
                os::InitializeWaitableHolder(std::addressof(this->request_stop_event_holder), this->request_stop_event.GetBase());
                os::LinkWaitableHolder(std::addressof(this->waitable_manager), std::addressof(this->request_stop_event_holder));
                os::InitializeWaitableHolder(std::addressof(this->notify_event_holder), this->notify_event.GetBase());
                os::LinkWaitableHolder(std::addressof(this->waitable_manager), std::addressof(this->notify_event_holder));
                os::InitializeWaitableManager(std::addressof(this->waitlist));
            }

            template<typename Interface>
            void RegisterObjectForServer(SharedPointer<Interface> static_object, Handle port_handle) {
                this->RegisterServerImpl(0, cmif::ServiceObjectHolder(std::move(static_object)), port_handle, false);
            }

            template<typename Interface>
            Result RegisterObjectForServer(SharedPointer<Interface> static_object, sm::ServiceName service_name, size_t max_sessions) {
                return this->RegisterServerImpl(0, cmif::ServiceObjectHolder(std::move(static_object)), service_name, max_sessions);
            }

            void RegisterServer(int port_index, Handle port_handle) {
                this->RegisterServerImpl(port_index, cmif::ServiceObjectHolder(), port_handle, false);
            }

            Result RegisterServer(int port_index, sm::ServiceName service_name, size_t max_sessions) {
                return this->RegisterServerImpl(port_index, cmif::ServiceObjectHolder(), service_name, max_sessions);
            }

            template<typename Interface>
            Result RegisterMitmServer(int port_index, sm::ServiceName service_name) {
                return this->template RegisterMitmServerImpl<Interface>(port_index, cmif::ServiceObjectHolder(), service_name);
            }

            /* Processing. */
            os::WaitableHolderType *WaitSignaled();

            void   ResumeProcessing();
            void   RequestStopProcessing();
            void   AddUserWaitableHolder(os::WaitableHolderType *waitable);

            Result Process(os::WaitableHolderType *waitable);
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
            os::Mutex resource_mutex;
            TYPED_STORAGE(Server) server_storages[MaxServers];
            bool server_allocated[MaxServers];
            TYPED_STORAGE(ServerSession) session_storages[MaxSessions];
            bool session_allocated[MaxSessions];
            u8 pointer_buffer_storage[0x10 + (MaxSessions * ManagerOptions::PointerBufferSize)];
            u8 saved_message_storage[0x10 + (MaxSessions * hipc::TlsMessageBufferSize)];
            uintptr_t pointer_buffers_start;
            uintptr_t saved_messages_start;

            /* Domain resources. */
            DomainStorage domain_storages[ManagerOptions::MaxDomains];
            bool domain_allocated[ManagerOptions::MaxDomains];
            DomainEntryStorage domain_entry_storages[ManagerOptions::MaxDomainObjects];
        private:
            constexpr inline size_t GetServerIndex(const Server *server) const {
                const size_t i = server - GetPointer(this->server_storages[0]);
                AMS_ABORT_UNLESS(i < MaxServers);
                return i;
            }

            constexpr inline size_t GetSessionIndex(const ServerSession *session) const {
                const size_t i = session - GetPointer(this->session_storages[0]);
                AMS_ABORT_UNLESS(i < MaxSessions);
                return i;
            }

            constexpr inline cmif::PointerAndSize GetObjectBySessionIndex(const ServerSession *session, uintptr_t start, size_t size) const {
                return cmif::PointerAndSize(start + this->GetSessionIndex(session) * size, size);
            }
        protected:
            virtual ServerSession *AllocateSession() override final {
                std::scoped_lock lk(this->resource_mutex);
                for (size_t i = 0; i < MaxSessions; i++) {
                    if (!this->session_allocated[i]) {
                        this->session_allocated[i] = true;
                        return GetPointer(this->session_storages[i]);
                    }
                }
                return nullptr;
            }

            virtual void FreeSession(ServerSession *session) override final {
                std::scoped_lock lk(this->resource_mutex);
                const size_t index = this->GetSessionIndex(session);
                AMS_ABORT_UNLESS(this->session_allocated[index]);
                this->session_allocated[index] = false;
            }

            virtual Server *AllocateServer() override final {
                std::scoped_lock lk(this->resource_mutex);
                for (size_t i = 0; i < MaxServers; i++) {
                    if (!this->server_allocated[i]) {
                        this->server_allocated[i] = true;
                        return GetPointer(this->server_storages[i]);
                    }
                }
                return nullptr;
            }

            virtual void DestroyServer(Server *server) override final {
                std::scoped_lock lk(this->resource_mutex);
                const size_t index = this->GetServerIndex(server);
                AMS_ABORT_UNLESS(this->server_allocated[index]);
                {
                    os::UnlinkWaitableHolder(server);
                    os::FinalizeWaitableHolder(server);
                    if (server->service_managed) {
                        if (server->is_mitm_server) {
                            R_ABORT_UNLESS(sm::mitm::UninstallMitm(server->service_name));
                        } else {
                            R_ABORT_UNLESS(sm::UnregisterService(server->service_name));
                        }
                        R_ABORT_UNLESS(svc::CloseHandle(server->port_handle));
                    }
                }
                this->server_allocated[index] = false;
            }

            virtual void *AllocateDomain() override final {
                std::scoped_lock lk(this->resource_mutex);
                for (size_t i = 0; i < ManagerOptions::MaxDomains; i++) {
                    if (!this->domain_allocated[i]) {
                        this->domain_allocated[i] = true;
                        return GetPointer(this->domain_storages[i]);
                    }
                }
                return nullptr;
            }

            virtual void FreeDomain(void *domain) override final {
                std::scoped_lock lk(this->resource_mutex);
                DomainStorage *ptr = static_cast<DomainStorage *>(domain);
                const size_t index = ptr - this->domain_storages;
                AMS_ABORT_UNLESS(index < ManagerOptions::MaxDomains);
                AMS_ABORT_UNLESS(this->domain_allocated[index]);
                this->domain_allocated[index] = false;
            }

            virtual cmif::PointerAndSize GetSessionPointerBuffer(const ServerSession *session) const override final {
                if constexpr (ManagerOptions::PointerBufferSize > 0) {
                    return this->GetObjectBySessionIndex(session, this->pointer_buffers_start, ManagerOptions::PointerBufferSize);
                } else {
                    return cmif::PointerAndSize();
                }
            }

            virtual cmif::PointerAndSize GetSessionSavedMessageBuffer(const ServerSession *session) const override final {
                return this->GetObjectBySessionIndex(session, this->saved_messages_start, hipc::TlsMessageBufferSize);
            }
        public:
            ServerManager() : ServerManagerBase(this->domain_entry_storages, ManagerOptions::MaxDomainObjects), resource_mutex(false) {
                /* Clear storages. */
                #define SF_SM_MEMCLEAR(obj) if constexpr (sizeof(obj) > 0) { std::memset(obj, 0, sizeof(obj)); }
                SF_SM_MEMCLEAR(this->server_storages);
                SF_SM_MEMCLEAR(this->server_allocated);
                SF_SM_MEMCLEAR(this->session_storages);
                SF_SM_MEMCLEAR(this->session_allocated);
                SF_SM_MEMCLEAR(this->pointer_buffer_storage);
                SF_SM_MEMCLEAR(this->saved_message_storage);
                SF_SM_MEMCLEAR(this->domain_allocated);
                #undef SF_SM_MEMCLEAR

                /* Set resource starts. */
                this->pointer_buffers_start = util::AlignUp(reinterpret_cast<uintptr_t>(this->pointer_buffer_storage), 0x10);
                this->saved_messages_start  = util::AlignUp(reinterpret_cast<uintptr_t>(this->saved_message_storage),  0x10);
            }

            ~ServerManager() {
                /* Close all sessions. */
                for (size_t i = 0; i < MaxSessions; i++) {
                    if (this->session_allocated[i]) {
                        this->CloseSessionImpl(GetPointer(this->session_storages[i]));
                    }
                }

                /* Close all servers. */
                for (size_t i = 0; i < MaxServers; i++) {
                    if (this->server_allocated[i]) {
                        this->DestroyServer(GetPointer(this->server_storages[i]));
                    }
                }
            }
    };

}
