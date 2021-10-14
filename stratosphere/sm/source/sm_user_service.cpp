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
#include "sm_user_service.hpp"
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    namespace {

        constexpr const u8 CmifResponseToQueryPointerBufferSize[] = {
            0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x53, 0x46, 0x43, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };

        static_assert(CmifCommandType_Request == 4);

        constexpr const u8 CmifExpectedRequestHeaderForRegisterClientAndDetachClient[] = {
            0x04, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00,
        };

        constexpr const u8 CmifResponseToRegisterClientAndDetachClient[] = {
            0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x53, 0x46, 0x43, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        constexpr const u8 CmifExpectedRequestHeaderForGetServiceHandleAndUnregisterService[] = {
            0x04, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00
        };

        constexpr const u8 CmifResponseToGetServiceHandleAndRegisterService[] = {
            0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x53, 0x46, 0x43, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        constexpr const u8 CmifResponseToUnregisterService[] = {
            0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x53, 0x46, 0x43, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        constexpr const u8 CmifExpectedRequestHeaderForRegisterService[] = {
            0x04, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00
        };

        static_assert(tipc::ResultRequestDeferred().GetValue() == 0xC823);
        constexpr const u8 CmifResponseToForceProcessorDeferral[] = {
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0xC8, 0x00, 0x00,
        };

    }

    Result UserService::ProcessDefaultServiceCommand(const svc::ipc::MessageBuffer &message_buffer) {
        /* Parse the hipc headers. */
        const svc::ipc::MessageBuffer::MessageHeader message_header(message_buffer);
        const svc::ipc::MessageBuffer::SpecialHeader special_header(message_buffer, message_header);

        /* Get the request tag. */
        const auto tag = message_header.GetTag();

        /* Handle the case where we received a control request. */
        if (tag == CmifCommandType_Control || tag == CmifCommandType_ControlWithContext) {
            /* The only control/control with context command which we support is QueryPointerBufferSize. */
            /* We do not have a pointer buffer, and so our pointer buffer size is zero. */
            /* Return the relevant hardcoded response. */
            std::memcpy(message_buffer.GetBufferForDebug(), CmifResponseToQueryPointerBufferSize, sizeof(CmifResponseToQueryPointerBufferSize));
            return ResultSuccess();
        }

        /* We only support request (with context), from this point forwards. */
        R_UNLESS((tag == CmifCommandType_Request || tag == CmifCommandType_RequestWithContext), tipc::ResultInvalidMethod());

        /* For ease of parsing, we will alter the tag to be Request when it is RequestWithContext. */
        if (tag == CmifCommandType_RequestWithContext) {
            message_buffer.Set(svc::ipc::MessageBuffer::MessageHeader(CmifCommandType_Request,
                                                                      message_header.GetHasSpecialHeader(),
                                                                      message_header.GetPointerCount(),
                                                                      message_header.GetSendCount(),
                                                                      message_header.GetReceiveCount(),
                                                                      message_header.GetExchangeCount(),
                                                                      message_header.GetRawCount(),
                                                                      message_header.GetReceiveListCount()));
        }

        /* NOTE: Nintendo only supports RegisterClient and GetServiceHandle. However, it would break */
        /* a substantial amount of homebrew system modules, if we were to not provide shims for some */
        /* other commands (RegisterService, and UnregisterService, in particular). As such, we will  */
        /* do the nice thing, and accommodate this by providing backwards compatibility shims for    */
        /* those commands. */

        /* Please note, though, that the atmosphere extensions are not shimmed, as performing all the required */
        /* parsing for that seems unreasonable. Also, if you are using atmosphere extensions, you are probably */
        /* used to my breaking shit regularly anyway, and I don't expect much of a fuss to be raised by you.   */
        /*  I invite you to demonstrate to me that this expectation does not reflect reality.                  */
        const u8 * const raw_message_buffer = static_cast<const u8 *>(message_buffer.GetBufferForDebug());
              u8 * const out_message_buffer = static_cast<u8 *>(message_buffer.GetBufferForDebug());

        if (std::memcmp(raw_message_buffer, CmifExpectedRequestHeaderForRegisterClientAndDetachClient, sizeof(CmifExpectedRequestHeaderForRegisterClientAndDetachClient)) == 0) {
            /* Get the command id. */
            const u32 command_id = *reinterpret_cast<const u32 *>(raw_message_buffer + 0x28);

            /* Get the client process id. */
            u64 client_process_id;
            std::memcpy(std::addressof(client_process_id), raw_message_buffer + 0xC, sizeof(client_process_id));

            if (command_id == 0) {
                /* Invoke the handler for RegisterClient. */
                R_ABORT_UNLESS(this->RegisterClient(tipc::ClientProcessId{ client_process_id }));
            } else if (command_id == 4) {
                /* Invoke the handler for DetachClient. */
                R_ABORT_UNLESS(this->DetachClient(tipc::ClientProcessId{ client_process_id }));
            } else {
                return tipc::ResultInvalidMethod();
            }

            /* Serialize output. */
            std::memcpy(out_message_buffer, CmifResponseToRegisterClientAndDetachClient, sizeof(CmifResponseToRegisterClientAndDetachClient));
        } else if (std::memcmp(raw_message_buffer, CmifExpectedRequestHeaderForGetServiceHandleAndUnregisterService, sizeof(CmifExpectedRequestHeaderForGetServiceHandleAndUnregisterService)) == 0) {
            /* Get the command id. */
            const u32 command_id = *reinterpret_cast<const u32 *>(raw_message_buffer + 0x18);

            /* Get the service_name. */
            sm::ServiceName service_name;
            std::memcpy(std::addressof(service_name), raw_message_buffer + 0x20, sizeof(service_name));

            if (command_id == 1) {
                /* Invoke the handler for GetServiceHandle. */
                svc::Handle out_handle = svc::InvalidHandle;
                const Result result = this->GetServiceHandle(std::addressof(out_handle), service_name);

                /* Serialize output. */
                if (!tipc::ResultRequestDeferred::Includes(result)) {
                    std::memcpy(out_message_buffer + 0x00, CmifResponseToGetServiceHandleAndRegisterService, sizeof(CmifResponseToGetServiceHandleAndRegisterService));
                    std::memcpy(out_message_buffer + 0x0C, std::addressof(out_handle), sizeof(out_handle));
                    std::memcpy(out_message_buffer + 0x18, std::addressof(result),  sizeof(result));
                } else {
                    std::memcpy(out_message_buffer, CmifResponseToForceProcessorDeferral, sizeof(CmifResponseToForceProcessorDeferral));
                }
            } else if (command_id == 3) {
                /* Invoke the handler for UnregisterService. */
                const Result result = this->UnregisterService(service_name);

                /* Serialize output. */
                std::memcpy(out_message_buffer + 0x00, CmifResponseToUnregisterService, sizeof(CmifResponseToUnregisterService));
                std::memcpy(out_message_buffer + 0x18, std::addressof(result),  sizeof(result));
            } else {
                return tipc::ResultInvalidMethod();
            }
        } else if (std::memcmp(raw_message_buffer, CmifExpectedRequestHeaderForRegisterService, sizeof(CmifExpectedRequestHeaderForRegisterService)) == 0) {
            /* Get the command id. */
            const u32 command_id = *reinterpret_cast<const u32 *>(raw_message_buffer + 0x18);

            /* Get the service_name. */
            sm::ServiceName service_name;
            std::memcpy(std::addressof(service_name), raw_message_buffer + 0x20, sizeof(service_name));

            /* Get "is light". */
            bool is_light;
            is_light = (raw_message_buffer[0x28] & 0x01) != 0;

            /* Get the max sessions. */
            u32 max_sessions;
            std::memcpy(std::addressof(max_sessions), raw_message_buffer + 0x2C, sizeof(max_sessions));

            if (command_id == 2) {
                /* Invoke the handler for RegisterService. */
                svc::Handle out_handle = svc::InvalidHandle;
                const Result result = this->RegisterService(std::addressof(out_handle), service_name, max_sessions, is_light);

                /* Serialize output. */
                std::memcpy(out_message_buffer + 0x00, CmifResponseToGetServiceHandleAndRegisterService, sizeof(CmifResponseToGetServiceHandleAndRegisterService));
                std::memcpy(out_message_buffer + 0x0C, std::addressof(out_handle), sizeof(out_handle));
                std::memcpy(out_message_buffer + 0x18, std::addressof(result),  sizeof(result));
            } else {
                return tipc::ResultInvalidMethod();
            }
        } else {
            return tipc::ResultInvalidMethod();
        }

        return ResultSuccess();
    }

}
