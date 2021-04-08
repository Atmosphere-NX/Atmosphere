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

namespace ams::_test {

    class UserInterfaceFacade {
        public:
            Result RegisterClient(const tipc::ClientProcessId &process_id);
            Result GetServiceHandle(tipc::OutMoveHandle out_h, sm::ServiceName service);
            Result RegisterService(tipc::OutMoveHandle out_h, sm::ServiceName service, u32 max_sessions, bool is_light);
            Result UnregisterService(sm::ServiceName service);
    };

    Result TestRegisterClient(UserInterfaceFacade *facade, const svc::ipc::MessageBuffer &message_buffer) {
        return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::RegisterClient, 16, Result, UserInterfaceFacade, const tipc::ClientProcessId &>(facade, message_buffer);
    }

    Result TestGetServiceHandle(UserInterfaceFacade *facade, const svc::ipc::MessageBuffer &message_buffer) {
        return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::GetServiceHandle, 17, Result, UserInterfaceFacade, tipc::OutMoveHandle, sm::ServiceName>(facade, message_buffer);
    }

    Result TestRegisterService(UserInterfaceFacade *facade, const svc::ipc::MessageBuffer &message_buffer) {
        return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::RegisterService, 18, Result, UserInterfaceFacade, tipc::OutMoveHandle, sm::ServiceName, u32, bool>(facade, message_buffer);
    }

    Result TestUnregisterService(UserInterfaceFacade *facade, const svc::ipc::MessageBuffer &message_buffer) {
        return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::UnregisterService, 19, Result, UserInterfaceFacade, sm::ServiceName>(facade, message_buffer);
    }

    Result TestManualDispatch(UserInterfaceFacade *facade) {
        svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());

        switch (svc::ipc::MessageBuffer::MessageHeader(message_buffer).GetTag()) {
            case 16:
                return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::RegisterClient, 16, Result, UserInterfaceFacade, const tipc::ClientProcessId &>(facade, message_buffer);
            case 17:
                return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::GetServiceHandle, 17, Result, UserInterfaceFacade, tipc::OutMoveHandle, sm::ServiceName>(facade, message_buffer);
            case 18:
                return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::RegisterService, 18, Result, UserInterfaceFacade, tipc::OutMoveHandle, sm::ServiceName, u32, bool>(facade, message_buffer);
            case 19:
                return tipc::impl::InvokeServiceCommandImpl<&UserInterfaceFacade::UnregisterService, 19, Result, UserInterfaceFacade, sm::ServiceName>(facade, message_buffer);
            default:
                return tipc::ResultInvalidMethod();
        }
    }


}