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

#define AMS_TEST_I_USER_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                \
    AMS_TIPC_METHOD_INFO(C, H,     0, Result, RegisterClient,                   (const tipc::ClientProcessId &client_process_id),                                           (client_process_id))                      \
    AMS_TIPC_METHOD_INFO(C, H,     1, Result, GetServiceHandle,                 (tipc::OutMoveHandle out_h, sm::ServiceName service),                                       (out_h, service))                         \
    AMS_TIPC_METHOD_INFO(C, H,     2, Result, RegisterService,                  (tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light),      (out_h, service, max_sessions, is_light)) \
    AMS_TIPC_METHOD_INFO(C, H,     3, Result, UnregisterService,                (sm::ServiceName service),                                                                  (service))

AMS_TIPC_DEFINE_INTERFACE(ams::_test::impl, IUserInterface, AMS_TEST_I_USER_INTERFACE_INTERFACE_INFO)


namespace ams::_test {

    class UserInterfaceFacade {
        public:
            Result RegisterClient(const tipc::ClientProcessId &process_id);
            Result GetServiceHandle(tipc::OutMoveHandle out_h, sm::ServiceName service);
            Result RegisterService(tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light);
            Result UnregisterService(sm::ServiceName service);
    };
    static_assert(impl::IsIUserInterface<UserInterfaceFacade>);


    Result TestManualDispatch(UserInterfaceFacade *facade) {
        svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());

        switch (svc::ipc::MessageBuffer::MessageHeader(message_buffer).GetTag()) {
            case 16:
                return tipc::impl::InvokeServiceCommandImpl<16, &UserInterfaceFacade::RegisterClient, UserInterfaceFacade>(facade, message_buffer);
            case 17:
                return tipc::impl::InvokeServiceCommandImpl<17, &UserInterfaceFacade::GetServiceHandle, UserInterfaceFacade>(facade, message_buffer);
            case 18:
                return tipc::impl::InvokeServiceCommandImpl<18, &UserInterfaceFacade::RegisterService, UserInterfaceFacade>(facade, message_buffer);
            case 19:
                return tipc::impl::InvokeServiceCommandImpl<19, &UserInterfaceFacade::UnregisterService, UserInterfaceFacade>(facade, message_buffer);
            default:
                return tipc::ResultInvalidMethod();
        }
    }

    using UserInterfaceObject = ::ams::tipc::ServiceObject<impl::IUserInterface, UserInterfaceFacade>;

    Result TestAutomaticDispatch(UserInterfaceObject *object) {
        return object->ProcessRequest();
    }


}