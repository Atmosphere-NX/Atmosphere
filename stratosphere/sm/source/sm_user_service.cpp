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
        if (this->has_initialized) {
            impl::OnClientDisconnected(this->process_id);
        }
    }

    Result UserService::RegisterClient(const sf::ClientProcessId &client_process_id) {
        this->process_id = client_process_id.GetValue();
        this->has_initialized = true;
        return ResultSuccess();
    }

    Result UserService::EnsureInitialized() {
        R_UNLESS(this->has_initialized, sm::ResultInvalidClient());
        return ResultSuccess();
    }

    Result UserService::GetServiceHandle(sf::OutMoveHandle out_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::GetServiceHandle(out_h.GetHandlePointer(), this->process_id, service);
    }

    Result UserService::RegisterService(sf::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light) {
        R_TRY(this->EnsureInitialized());
        return impl::RegisterService(out_h.GetHandlePointer(), this->process_id, service, max_sessions, is_light);
    }

    Result UserService::UnregisterService(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::UnregisterService(this->process_id, service);
    }

    Result UserService::DetachClient(const sf::ClientProcessId &client_process_id) {
        this->has_initialized = false;
        return ResultSuccess();
    }

    Result UserService::AtmosphereInstallMitm(sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::InstallMitm(srv_h.GetHandlePointer(), qry_h.GetHandlePointer(), this->process_id, service);
    }

    Result UserService::AtmosphereUninstallMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::UninstallMitm(this->process_id, service);
    }

    Result UserService::AtmosphereAcknowledgeMitmSession(sf::Out<MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::AcknowledgeMitmSession(client_info.GetPointer(), fwd_h.GetHandlePointer(), this->process_id, service);
    }

    Result UserService::AtmosphereHasMitm(sf::Out<bool> out, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::HasMitm(out.GetPointer(), service);
    }

    Result UserService::AtmosphereWaitMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::WaitMitm(service);
    }

    Result UserService::AtmosphereDeclareFutureMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::DeclareFutureMitm(this->process_id, service);
    }

    Result UserService::AtmosphereClearFutureMitm(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::ClearFutureMitm(this->process_id, service);
    }


    Result UserService::AtmosphereHasService(sf::Out<bool> out, ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::HasService(out.GetPointer(), service);
    }

    Result UserService::AtmosphereWaitService(ServiceName service) {
        R_TRY(this->EnsureInitialized());
        return impl::WaitService(service);
    }

}
