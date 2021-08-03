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
#include "sm_tipc_server.hpp"
#include "sm_user_service.hpp"
#include "sm_manager_service.hpp"
#include "sm_wait_list.hpp"
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    namespace {

        /* Server limit definitions. */
        enum PortIndex : size_t {
            PortIndex_Manager,
            PortIndex_User,
            PortIndex_Count,
        };

        constexpr inline size_t NumTipcPorts            = static_cast<size_t>(PortIndex_Count);
        constexpr inline size_t MaxSessionsManager      = 1;
        constexpr inline size_t MaxSessionsUser         = 0x50 + 8 - MaxSessionsManager;

        constexpr inline size_t MaxSessionsTotal   = MaxSessionsManager + MaxSessionsUser;


        static_assert(MaxSessionsTotal % NumTipcPorts == 0);

        /* Define WaitList class. */
        class WaitList {
            public:
                using Key = sm::ServiceName;
            private:
                struct Entry {
                    sm::ServiceName service_name{sm::InvalidServiceName};
                    tipc::WaitableObject object{};
                    u8 message_buffer[svc::ipc::MessageBufferSize];
                };
            private:
                Entry m_entries[MaxSessionsTotal / NumTipcPorts]{};
                Entry *m_processing_entry{};
            public:
                constexpr WaitList() = default;
            public:
                Result StartRegisterRetry(sm::ServiceName service_name) {
                    /* Check that we're not already processing a retry. */
                    AMS_ABORT_UNLESS(m_processing_entry == nullptr);

                    /* Find a free entry. */
                    Entry *free_entry = nullptr;
                    for (auto &entry : m_entries) {
                        if (entry.service_name == InvalidServiceName) {
                            free_entry = std::addressof(entry);
                            break;
                        }
                    }

                    /* Verify that we found a free entry. */
                    R_UNLESS(free_entry != nullptr, sm::ResultOutOfProcesses());

                    /* Populate the entry. */
                    free_entry->service_name = service_name;
                    std::memcpy(free_entry->message_buffer, svc::ipc::GetMessageBuffer(), util::size(free_entry->message_buffer));

                    /* Set the processing entry. */
                    m_processing_entry = free_entry;

                    /* Return the special request deferral result. */
                    return tipc::ResultRequestDeferred();
                }

                void ProcessRegisterRetry(tipc::WaitableObject &object) {
                    /* Verify that we have a processing entry. */
                    AMS_ABORT_UNLESS(m_processing_entry != nullptr);

                    /* Set the entry's object. */
                    m_processing_entry->object = object;

                    /* Clear our processing entry. */
                    m_processing_entry = nullptr;
                }

                bool TestResume(sm::ServiceName service_name) {
                    /* Check that we have a matching service name. */
                    for (const auto &entry : m_entries) {
                        if (entry.service_name == service_name) {
                            return true;
                        }
                    }

                    return false;
                }

                template<typename PortManagerType>
                void Resume(sm::ServiceName service_name, PortManagerType *port_manager) {
                    /* Resume (and clear) all matching entries. */
                    for (auto &entry : m_entries) {
                        if (entry.service_name == service_name) {
                            /* Copy the saved message buffer. */
                            std::memcpy(svc::ipc::GetMessageBuffer(), entry.message_buffer, svc::ipc::MessageBufferSize);

                            /* Resume the request. */
                            R_TRY_CATCH(port_manager->ProcessRequest(entry.object)) {
                                R_CATCH(tipc::ResultRequestDeferred) {
                                    this->ProcessRegisterRetry(entry.object);
                                }
                            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                            /* Clear the entry's service name. */
                            entry.service_name = sm::InvalidServiceName;
                        }
                    }
                }
        };

        /* Define port metadata. */
        using UserPortMeta         = tipc::PortMeta<MaxSessionsUser,         impl::IUserInterface,         UserService,         tipc::SlabAllocator>;
        using ManagerPortMeta      = tipc::PortMeta<MaxSessionsManager,      impl::IManagerInterface,      ManagerService,      tipc::SingletonAllocator>;

        /* Define server manager global. */
        using ServerManager = tipc::ServerManagerWithDeferral<sm::WaitList, ManagerPortMeta, UserPortMeta>;

        ServerManager g_server_manager;

    }

    void InitializeTipcServer() {
        /* Initialize the server manager. */
        g_server_manager.Initialize();

        /* Create the handles for our ports. */
        svc::Handle user_port_handle = svc::InvalidHandle, manager_port_handle = svc::InvalidHandle;
        {
            /* Create the user port handle. */
            R_ABORT_UNLESS(svc::ManageNamedPort(std::addressof(user_port_handle), "sm:", MaxSessionsUser));

            /* Create the manager port handle. */
            R_ABORT_UNLESS(impl::RegisterServiceForSelf(std::addressof(manager_port_handle), sm::ServiceName::Encode("sm:m"), MaxSessionsManager));

            /* TODO: Debug Monitor port? */
        }

        /* Register the ports. */
        g_server_manager.RegisterPort<PortIndex_Manager>(manager_port_handle);
        g_server_manager.RegisterPort<PortIndex_User   >(user_port_handle);
    }

    void LoopProcessTipcServer() {
        /* Loop processing the server on all threads. */
        g_server_manager.LoopAuto();
    }

    Result StartRegisterRetry(sm::ServiceName service_name) {
        /* Get the port manager from where it's saved in TLS. */
        auto *port_manager = reinterpret_cast<typename ServerManager::PortManagerBase *>(os::GetTlsValue(g_server_manager.GetTlsSlot()));

        /* Register the retry. */
        return port_manager->StartRegisterRetry(service_name);
    }

    void TriggerResume(sm::ServiceName service_name) {
        /* Trigger a resumption. */
        g_server_manager.TriggerResume(service_name);
    }

}
