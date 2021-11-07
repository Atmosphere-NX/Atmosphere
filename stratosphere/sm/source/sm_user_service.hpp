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
#include <stratosphere.hpp>
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    /* Service definition. */
    class UserService : public tipc::DeferrableBase<sm::impl::IUserInterface, /* Maximum deferrable CMIF message size: */ 0x20 + util::AlignUp(sizeof(sm::ServiceName), sizeof(u32))> {
        private:
            os::ProcessId m_process_id;
            bool m_initialized;
        public:
            UserService() : m_process_id{os::InvalidProcessId}, m_initialized{false} { /* ... */ }
            ~UserService() {
                if (m_initialized) {
                    impl::OnClientDisconnected(m_process_id);
                }
            }
        public:
            /* Official commands. */
            Result RegisterClient(const tipc::ClientProcessId client_process_id) {
                m_process_id  = client_process_id.value;
                m_initialized = true;
                return ResultSuccess();
            }

            Result GetServiceHandle(tipc::OutMoveHandle out_h, ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return this->RegisterRetryIfDeferred(service, [&]() ALWAYS_INLINE_LAMBDA -> Result {
                    return impl::GetServiceHandle(out_h.GetPointer(), m_process_id, service);
                });
            }

            Result RegisterService(tipc::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::RegisterService(out_h.GetPointer(), m_process_id, service, max_sessions, is_light);
            }

            Result UnregisterService(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::UnregisterService(m_process_id, service);
            }

            Result DetachClient(const tipc::ClientProcessId client_process_id) {
                AMS_UNUSED(client_process_id);

                m_initialized = false;
                return ResultSuccess();
            }

            /* Atmosphere commands. */
            Result AtmosphereInstallMitm(tipc::OutMoveHandle srv_h, tipc::OutMoveHandle qry_h, ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return this->RegisterRetryIfDeferred(service, [&]() ALWAYS_INLINE_LAMBDA -> Result {
                    return impl::InstallMitm(srv_h.GetPointer(), qry_h.GetPointer(), m_process_id, service);
                });
            }

            Result AtmosphereUninstallMitm(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::UninstallMitm(m_process_id, service);
            }

            Result AtmosphereAcknowledgeMitmSession(tipc::Out<MitmProcessInfo> client_info, tipc::OutMoveHandle fwd_h, ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::AcknowledgeMitmSession(client_info.GetPointer(), fwd_h.GetPointer(), m_process_id, service);
            }

            Result AtmosphereHasMitm(tipc::Out<bool> out, ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::HasMitm(out.GetPointer(), service);
            }

            Result AtmosphereWaitMitm(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return this->RegisterRetryIfDeferred(service, [&]() ALWAYS_INLINE_LAMBDA -> Result {
                    return impl::WaitMitm(service);
                });
            }

            Result AtmosphereDeclareFutureMitm(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::DeclareFutureMitm(m_process_id, service);
            }

            Result AtmosphereClearFutureMitm(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::ClearFutureMitm(m_process_id, service);
            }

            Result AtmosphereHasService(tipc::Out<bool> out, ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return impl::HasService(out.GetPointer(), service);
            }

            Result AtmosphereWaitService(ServiceName service) {
                R_UNLESS(m_initialized, sm::ResultInvalidClient());
                return this->RegisterRetryIfDeferred(service, [&]() ALWAYS_INLINE_LAMBDA -> Result {
                    return impl::WaitService(service);
                });
            }
        public:
            /* Backwards compatibility layer for cmif. */
            Result ProcessDefaultServiceCommand(const svc::ipc::MessageBuffer &message_buffer);
    };
    static_assert(sm::impl::IsIUserInterface<UserService>);
    static_assert(tipc::IsDeferrable<UserService>);
    /* TODO: static assert that this is a tipc interface with default prototyping. */

}
