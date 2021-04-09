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
#include <stratosphere/tipc/impl/tipc_autogen_interface_macros.hpp>

#define AMS_TEST_I_USER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                           \
    AMS_TIPC_METHOD_INFO(C, H,     0, Result, RegisterClient,                   (const tipc::ClientProcessId &client_process_id),                                      (client_process_id))                      \
    AMS_TIPC_METHOD_INFO(C, H,     1, Result, GetServiceHandle,                 (tipc::OutMoveHandle out_h, sm::ServiceName service),                                  (out_h, service))                         \
    AMS_TIPC_METHOD_INFO(C, H,     2, Result, RegisterService,                  (tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light), (out_h, service, max_sessions, is_light)) \
    AMS_TIPC_METHOD_INFO(C, H,     3, Result, UnregisterService,                (sm::ServiceName service),                                                             (service))

AMS_TIPC_DEFINE_INTERFACE(ams::_test::impl, IUserInterface, AMS_TEST_I_USER_INTERFACE_INTERFACE_INFO)

#define AMS_TEST_I_MANAGER_INTERFACE_INTERFACE_INFO(C, H) \
    AMS_TIPC_METHOD_INFO(C, H,     0, Result, RegisterProcess,           (os::ProcessId process_id, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac), (process_id, acid_sac, aci_sac)) \
    AMS_TIPC_METHOD_INFO(C, H,     1, Result, UnregisterProcess,         (os::ProcessId process_id),                                                              (process_id))

AMS_TIPC_DEFINE_INTERFACE(ams::_test::impl, IManagerInterface, AMS_TEST_I_MANAGER_INTERFACE_INTERFACE_INFO)


namespace ams::_test {

    class UserInterfaceFacade {
        public:
            Result RegisterClient(const tipc::ClientProcessId &process_id);
            Result GetServiceHandle(tipc::OutMoveHandle out_h, sm::ServiceName service);
            Result RegisterService(tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light);
            Result UnregisterService(sm::ServiceName service);

            Result ProcessDefaultServiceCommand(const svc::ipc::MessageBuffer &message_buffer);
    };
    static_assert(impl::IsIUserInterface<UserInterfaceFacade>);

    class ManagerInterfaceFacade {
        public:
            Result RegisterProcess(os::ProcessId process_id, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac);
            Result UnregisterProcess(os::ProcessId process_id);
    };
    static_assert(impl::IsIManagerInterface<ManagerInterfaceFacade>);

    using UserInterfaceObject = ::ams::tipc::ServiceObject<impl::IUserInterface, UserInterfaceFacade>;

    Result TestAutomaticDispatch(UserInterfaceObject *object) {
        return object->ProcessRequest();
    }

    using ManagerInterfaceObject = ::ams::tipc::ServiceObject<impl::IManagerInterface, ManagerInterfaceFacade>;

    Result TestManagerDispatch(ManagerInterfaceObject *object) {
        return object->ProcessRequest();
    }


}