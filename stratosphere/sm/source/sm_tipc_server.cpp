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

        /* Define port metadata. */
        using UserPortMeta         = tipc::PortMeta<MaxSessionsUser,    impl::IUserInterface,    UserService,    tipc::SlabAllocator>;
        using ManagerPortMeta      = tipc::PortMeta<MaxSessionsManager, impl::IManagerInterface, ManagerService, tipc::SingletonAllocator>;

        /* Define server manager global. */
        using ServerManager = tipc::ServerManager<ManagerPortMeta, UserPortMeta>;

        ServerManager g_server_manager;

    }

    void InitializeTipcServer() {
        /* Initialize the server manager. */
        g_server_manager.Initialize();

        /* Create the handles for our ports. */
        os::NativeHandle user_port_handle, manager_port_handle;
        {
            /* Create the user port handle. */
            R_ABORT_UNLESS(svc::ManageNamedPort(std::addressof(user_port_handle), "sm:", MaxSessionsUser));

            /* Create the manager port handle. */
            R_ABORT_UNLESS(impl::RegisterServiceForSelf(std::addressof(manager_port_handle), sm::ServiceName::Encode("sm:m"), MaxSessionsManager));
        }

        /* Register the ports. */
        g_server_manager.RegisterPort<PortIndex_Manager>(manager_port_handle);
        g_server_manager.RegisterPort<PortIndex_User   >(user_port_handle);
    }

    void LoopProcessTipcServer() {
        /* Loop processing the server on all threads. */
        g_server_manager.LoopAuto(AMS_GET_SYSTEM_THREAD_PRIORITY(sm, DispatcherThread), AMS_GET_SYSTEM_THREAD_NAME(sm, DispatcherThread));
    }

    void TriggerResume(sm::ServiceName service_name) {
        /* Trigger a resumption. */
        g_server_manager.TriggerResume(service_name);
    }

}
