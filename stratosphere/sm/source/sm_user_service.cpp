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
#include "sm_user_service.hpp"
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    UserService::~UserService() {
        if (m_initialized) {
            impl::OnClientDisconnected(m_process_id);
        }
    }

    Result UserService::RegisterClient(const tipc::ClientProcessId client_process_id) {
        m_process_id  = client_process_id.value;
        m_initialized = true;
        return ResultSuccess();
    }

    Result UserService::EnsureInitialized() {
        R_UNLESS(m_initialized, sm::ResultInvalidClient());
        return ResultSuccess();
    }

    Result UserService::GetServiceHandle(tipc::OutMoveHandle out_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::GetServiceHandle(out_h.GetHandlePointer(), m_process_id, service);
    }

    Result UserService::RegisterService(tipc::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light) {
        R_TRY(this->EnsureInitialized());
        return impl::RegisterService(out_h.GetHandlePointer(), m_process_id, service, max_sessions, is_light);
    }

    Result UserService::UnregisterService(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::UnregisterService(m_process_id, service);
    }

    Result UserService::DetachClient(const tipc::ClientProcessId client_process_id) {
        m_initialized = false;
        return ResultSuccess();
    }

    Result UserService::AtmosphereInstallMitm(tipc::OutMoveHandle srv_h, tipc::OutMoveHandle qry_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::InstallMitm(srv_h.GetHandlePointer(), qry_h.GetHandlePointer(), m_process_id, service);
    }

    Result UserService::AtmosphereUninstallMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::UninstallMitm(m_process_id, service);
    }

    Result UserService::AtmosphereAcknowledgeMitmSession(tipc::Out<MitmProcessInfo> client_info, tipc::OutMoveHandle fwd_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::AcknowledgeMitmSession(client_info.GetPointer(), fwd_h.GetHandlePointer(), m_process_id, service);
    }

    Result UserService::AtmosphereHasMitm(tipc::Out<bool> out, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::HasMitm(out.GetPointer(), service);
    }

    Result UserService::AtmosphereWaitMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::WaitMitm(service);
    }

    Result UserService::AtmosphereDeclareFutureMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::DeclareFutureMitm(m_process_id, service);
    }

    Result UserService::AtmosphereClearFutureMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::ClearFutureMitm(m_process_id, service);
    }

    Result UserService::AtmosphereHasService(tipc::Out<bool> out, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::HasService(out.GetPointer(), service);
    }

    Result UserService::AtmosphereWaitService(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::WaitService(service);
    }

}

/* Include the backwards compatibility shim for CMIF. */
#include "sm_user_service_cmif_shim.inc"
