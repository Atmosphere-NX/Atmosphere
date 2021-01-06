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
        private:
            class ServerBase : public os::WaitableHolderType {
                friend class ServerManagerBase;
                template<size_t, typename, size_t>
                friend class ServerManager;
                NON_COPYABLE(ServerBase);
                NON_MOVEABLE(ServerBase);
                protected:
                    cmif::ServiceObjectHolder static_object;
                    ::Handle port_handle;
                    sm::ServiceName service_name;
                    bool service_managed;
                public:
                    ServerBase(Handle ph, sm::ServiceName sn, bool m, cmif::ServiceObjectHolder &&sh) :
                        static_object(std::move(sh)), port_handle(ph), service_name(sn), service_managed(m)
                    {
                        hipc::AttachWaitableHolderForAccept(this, ph);
                    }

                    virtual ~ServerBase() = 0;
                    virtual void CreateSessionObjectHolder(cmif::ServiceObjectHolder *out_obj, std::shared_ptr<::Service> *out_fsrv) const = 0;
            };

            template<typename Interface, auto MakeShared>
            class Server : public ServerBase {
                NON_COPYABLE(Server);
                NON_MOVEABLE(Server);
                private:
                    static constexpr bool IsMitmServer = sf::IsMitmServiceObject<Interface>;
                public:
                    Server(Handle ph, sm::ServiceName sn, bool m, cmif::ServiceObjectHolder &&sh) : ServerBase(ph, sn, m, std::forward<cmif::ServiceObjectHolder>(sh)) {
                        /* ... */
                    }

                    virtual ~Server() override {
                        if (this->service_managed) {
                            if constexpr (IsMitmServer) {
                                R_ABORT_UNLESS(sm::mitm::UninstallMitm(this->service_name));
                            } else {
                                R_ABORT_UNLESS(sm::UnregisterService(this->service_name));
                            }
                            R_ABORT_UNLESS(svc::CloseHandle(this->port_handle));
                        }
                    }

                    virtual void CreateSessionObjectHolder(cmif::ServiceObjectHolder *out_obj, std::shared_ptr<::Service> *out_fsrv) const override final {
                        /* If we're serving a static object, use it. */
                        if (this->static_object) {
                            *out_obj = std::move(this->static_object.Clone());
                            *out_fsrv = nullptr;
                            return;
                        }

                        /* Otherwise, we're either a mitm session or a non-mitm session. */
                        if constexpr (IsMitmServer) {
                            /* Custom deleter ensures that nothing goes awry. */
                            std::shared_ptr<::Service> forward_service = std::move(ServerSession::CreateForwardService());

                            /* Get mitm forward session. */
                            sm::MitmProcessInfo client_info;
                            R_ABORT_UNLESS(sm::mitm::AcknowledgeSession(forward_service.get(), &client_info, this->service_name));

                            *out_obj = std::move(cmif::ServiceObjectHolder(std::move(MakeShared(std::shared_ptr<::Service>(forward_service), client_info))));
                            *out_fsrv = std::move(forward_service);
                        } else {
                            *out_obj = std::move(cmif::ServiceObjectHolder(std::move(MakeShared())));
                            *out_fsrv = nullptr;
                        }
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

            template<typename Interface, auto MakeShared>
            void RegisterServerImpl(Handle port_handle, sm::ServiceName service_name, bool managed, cmif::ServiceObjectHolder &&static_holder) {
                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ABORT_UNLESS(server != nullptr);
                new (server) Server<Interface, MakeShared>(port_handle, service_name, managed, std::forward<cmif::ServiceObjectHolder>(static_holder));

                if constexpr (!sf::IsMitmServiceObject<Interface>) {
                    /* Non-mitm server. */
                    os::SetWaitableHolderUserData(server, static_cast<uintptr_t>(UserDataTag::Server));
                } else {
                    /* Mitm server. */
                    os::SetWaitableHolderUserData(server, static_cast<uintptr_t>(UserDataTag::MitmServer));
                }

                os::LinkWaitableHolder(std::addressof(this->waitable_manager), server);
            }

            template<typename Interface, typename ServiceImpl>
            static constexpr inline std::shared_ptr<Interface> MakeSharedMitm(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &client_info) {
                return sf::MakeShared<Interface, ServiceImpl>(std::forward<std::shared_ptr<::Service>>(s), client_info);
            }

            Result InstallMitmServerImpl(Handle *out_port_handle, sm::ServiceName service_name, MitmQueryFunction query_func);
        protected:
            virtual ServerBase *AllocateServer() = 0;
            virtual void DestroyServer(ServerBase *server)  = 0;
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

            template<typename Interface, typename ServiceImpl, auto MakeShared = sf::MakeShared<Interface, ServiceImpl>> requires (sf::IsServiceObject<Interface> && !sf::IsMitmServiceObject<Interface>)
            void RegisterServer(Handle port_handle, std::shared_ptr<Interface> static_object = nullptr) {
                /* Register server. */
                cmif::ServiceObjectHolder static_holder;
                if (static_object != nullptr) {
                    static_holder = cmif::ServiceObjectHolder(std::move(static_object));
                }
                this->RegisterServerImpl<Interface, MakeShared>(port_handle, sm::InvalidServiceName, false, std::move(static_holder));
            }

            template<typename Interface, typename ServiceImpl, auto MakeShared = sf::MakeShared<Interface, ServiceImpl>> requires (sf::IsServiceObject<Interface> && !sf::IsMitmServiceObject<Interface>)
            Result RegisterServer(sm::ServiceName service_name, size_t max_sessions, std::shared_ptr<Interface> static_object = nullptr) {
                /* Register service. */
                Handle port_handle;
                R_TRY(sm::RegisterService(&port_handle, service_name, max_sessions, false));

                /* Register server. */
                cmif::ServiceObjectHolder static_holder;
                if (static_object != nullptr) {
                    static_holder = cmif::ServiceObjectHolder(std::move(static_object));
                }
                this->RegisterServerImpl<Interface, MakeShared>(port_handle, service_name, true, std::move(static_holder));
                return ResultSuccess();
            }

            template<typename Interface, typename ServiceImpl, auto MakeShared = MakeSharedMitm<Interface, ServiceImpl>>
                requires (sf::IsMitmServiceObject<Interface> && sf::IsMitmServiceImpl<ServiceImpl>)
            Result RegisterMitmServer(sm::ServiceName service_name) {
                /* Install mitm service. */
                Handle port_handle;
                R_TRY(this->InstallMitmServerImpl(&port_handle, service_name, &ServiceImpl::ShouldMitm));

                this->RegisterServerImpl<Interface, MakeShared>(port_handle, service_name, true, cmif::ServiceObjectHolder());
                return ResultSuccess();
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
            TYPED_STORAGE(ServerBase) server_storages[MaxServers];
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
            constexpr inline size_t GetServerIndex(const ServerBase *server) const {
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

            virtual ServerBase *AllocateServer() override final {
                std::scoped_lock lk(this->resource_mutex);
                for (size_t i = 0; i < MaxServers; i++) {
                    if (!this->server_allocated[i]) {
                        this->server_allocated[i] = true;
                        return GetPointer(this->server_storages[i]);
                    }
                }
                return nullptr;
            }

            virtual void DestroyServer(ServerBase *server) override final {
                std::scoped_lock lk(this->resource_mutex);
                const size_t index = this->GetServerIndex(server);
                AMS_ABORT_UNLESS(this->server_allocated[index]);
                server->~ServerBase();
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
