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
            class ServerBase : public os::WaitableHolder {
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
                        os::WaitableHolder(ph), static_object(std::move(sh)), port_handle(ph), service_name(sn), service_managed(m)
                    {
                        /* ... */
                    }

                    virtual ~ServerBase() = 0;
                    virtual void CreateSessionObjectHolder(cmif::ServiceObjectHolder *out_obj, std::shared_ptr<::Service> *out_fsrv) const = 0;
            };

            template<typename ServiceImpl, auto MakeShared = std::make_shared<ServiceImpl>>
            class Server : public ServerBase {
                NON_COPYABLE(Server);
                NON_MOVEABLE(Server);
                private:
                    static constexpr bool IsMitmServer = ServiceObjectTraits<ServiceImpl>::IsMitmServiceObject;
                public:
                    Server(Handle ph, sm::ServiceName sn, bool m, cmif::ServiceObjectHolder &&sh) : ServerBase(ph, sn, m, std::forward<cmif::ServiceObjectHolder>(sh)) {
                        /* ... */
                    }

                    virtual ~Server() override {
                        if (this->service_managed) {
                            if constexpr (IsMitmServer) {
                                R_ASSERT(sm::mitm::UninstallMitm(this->service_name));
                            } else {
                                R_ASSERT(sm::UnregisterService(this->service_name));
                            }
                            R_ASSERT(svcCloseHandle(this->port_handle));
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
                            /* TODO: Should this just be a custom wrapper object? */
                            std::shared_ptr<::Service> forward_service = std::move(ServerSession::CreateForwardService());

                            /* Get mitm forward session. */
                            sm::MitmProcessInfo client_info;
                            R_ASSERT(sm::mitm::AcknowledgeSession(forward_service.get(), &client_info, this->service_name));

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
            os::WaitableManager waitable_manager;
            os::Event request_stop_event;
            os::WaitableHolder request_stop_event_holder;
            os::Event notify_event;
            os::WaitableHolder notify_event_holder;

            os::Mutex waitable_selection_mutex;

            os::Mutex waitlist_mutex;
            os::WaitableManager waitlist;

            os::Mutex deferred_session_mutex;
            using DeferredSessionList = typename util::IntrusiveListMemberTraits<&ServerSession::deferred_list_node>::ListType;
            DeferredSessionList deferred_session_list;
        private:
            virtual void RegisterSessionToWaitList(ServerSession *session) override final;
            void RegisterToWaitList(os::WaitableHolder *holder);
            void ProcessWaitList();

            bool WaitAndProcessImpl();

            Result ProcessForServer(os::WaitableHolder *holder);
            Result ProcessForMitmServer(os::WaitableHolder *holder);
            Result ProcessForSession(os::WaitableHolder *holder);

            void   ProcessDeferredSessions();

            template<typename ServiceImpl, auto MakeShared = std::make_shared<ServiceImpl>>
            void RegisterServerImpl(Handle port_handle, sm::ServiceName service_name, bool managed, cmif::ServiceObjectHolder &&static_holder) {
                /* Allocate server memory. */
                auto *server = this->AllocateServer();
                AMS_ASSERT(server != nullptr);
                new (server) Server<ServiceImpl, MakeShared>(port_handle, service_name, managed, std::forward<cmif::ServiceObjectHolder>(static_holder));

                if constexpr (!ServiceObjectTraits<ServiceImpl>::IsMitmServiceObject) {
                    /* Non-mitm server. */
                    server->SetUserData(static_cast<uintptr_t>(UserDataTag::Server));
                } else {
                    /* Mitm server. */
                    server->SetUserData(static_cast<uintptr_t>(UserDataTag::MitmServer));
                }

                this->waitable_manager.LinkWaitableHolder(server);
            }

            template<typename ServiceImpl>
            static constexpr inline std::shared_ptr<ServiceImpl> MakeSharedMitm(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &client_info) {
                return std::make_shared<ServiceImpl>(std::forward<std::shared_ptr<::Service>>(s), client_info);
            }

            Result InstallMitmServerImpl(Handle *out_port_handle, sm::ServiceName service_name, MitmQueryFunction query_func);
        protected:
            virtual ServerBase *AllocateServer() = 0;
            virtual void DestroyServer(ServerBase *server)  = 0;
        public:
            ServerManagerBase(DomainEntryStorage *entry_storage, size_t entry_count) :
                ServerDomainSessionManager(entry_storage, entry_count),
                request_stop_event(false), request_stop_event_holder(&request_stop_event),
                notify_event(false), notify_event_holder(&notify_event)
            {
                /* Link waitables. */
                this->waitable_manager.LinkWaitableHolder(&this->request_stop_event_holder);
                this->waitable_manager.LinkWaitableHolder(&this->notify_event_holder);
            }

            template<typename ServiceImpl, auto MakeShared = std::make_shared<ServiceImpl>>
            void RegisterServer(Handle port_handle, std::shared_ptr<ServiceImpl> static_object = nullptr) {
                static_assert(!ServiceObjectTraits<ServiceImpl>::IsMitmServiceObject, "RegisterServer requires non-mitm object. Use RegisterMitmServer instead.");
                /* Register server. */
                cmif::ServiceObjectHolder static_holder;
                if (static_object != nullptr) {
                    static_holder = cmif::ServiceObjectHolder(std::move(static_object));
                }
                this->RegisterServerImpl<ServiceImpl, MakeShared>(port_handle, sm::InvalidServiceName, false, std::move(static_holder));
            }

            template<typename ServiceImpl, auto MakeShared = std::make_shared<ServiceImpl>>
            Result RegisterServer(sm::ServiceName service_name, size_t max_sessions, std::shared_ptr<ServiceImpl> static_object = nullptr) {
                static_assert(!ServiceObjectTraits<ServiceImpl>::IsMitmServiceObject, "RegisterServer requires non-mitm object. Use RegisterMitmServer instead.");

                /* Register service. */
                Handle port_handle;
                R_TRY(sm::RegisterService(&port_handle, service_name, max_sessions, false));

                /* Register server. */
                cmif::ServiceObjectHolder static_holder;
                if (static_object != nullptr) {
                    static_holder = cmif::ServiceObjectHolder(std::move(static_object));
                }
                this->RegisterServerImpl<ServiceImpl, MakeShared>(port_handle, service_name, true, std::move(static_holder));
                return ResultSuccess();
            }

            template<typename ServiceImpl, auto MakeShared = MakeSharedMitm<ServiceImpl>>
            Result RegisterMitmServer(sm::ServiceName service_name) {
                static_assert(ServiceObjectTraits<ServiceImpl>::IsMitmServiceObject, "RegisterMitmServer requires mitm object. Use RegisterServer instead.");

                /* Install mitm service. */
                Handle port_handle;
                R_TRY(this->InstallMitmServerImpl(&port_handle, service_name, &ServiceImpl::ShouldMitm));

                this->RegisterServerImpl<ServiceImpl, MakeShared>(port_handle, service_name, true, cmif::ServiceObjectHolder());
                return ResultSuccess();
            }

            /* Processing. */
            os::WaitableHolder *WaitSignaled();

            void   ResumeProcessing();
            void   RequestStopProcessing();
            void   AddUserWaitableHolder(os::WaitableHolder *waitable);

            Result Process(os::WaitableHolder *waitable);
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
                AMS_ASSERT(i < MaxServers);
                return i;
            }

            constexpr inline size_t GetSessionIndex(const ServerSession *session) const {
                const size_t i = session - GetPointer(this->session_storages[0]);
                AMS_ASSERT(i < MaxSessions);
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
                AMS_ASSERT(this->session_allocated[index]);
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
                AMS_ASSERT(this->server_allocated[index]);
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
                AMS_ASSERT(index < ManagerOptions::MaxDomains);
                AMS_ASSERT(this->domain_allocated[index]);
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
            ServerManager() : ServerManagerBase(this->domain_entry_storages, ManagerOptions::MaxDomainObjects) {
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
