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
#include "ro_service.hpp"
#include "impl/ro_service_impl.hpp"

namespace ams::ro {

    void SetDevelopmentHardware(bool is_development_hardware) {
        impl::SetDevelopmentHardware(is_development_hardware);
    }

    void SetDevelopmentFunctionEnabled(bool is_development_function_enabled) {
        impl::SetDevelopmentFunctionEnabled(is_development_function_enabled);
    }

    Service::Service(ModuleType t) : context_id(impl::InvalidContextId), type(t) {
        /* ... */
    }

    Service::~Service() {
        impl::UnregisterProcess(this->context_id);
    }

    Result Service::LoadNro(sf::Out<u64> load_address, const sf::ClientProcessId &client_pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::LoadNro(load_address.GetPointer(), this->context_id, nro_address, nro_size, bss_address, bss_size);
    }

    Result Service::UnloadNro(const sf::ClientProcessId &client_pid, u64 nro_address) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::UnloadNro(this->context_id, nro_address);
    }

    Result Service::LoadNrr(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::LoadNrr(this->context_id, INVALID_HANDLE, nrr_address, nrr_size, ModuleType::ForSelf, true);
    }

    Result Service::UnloadNrr(const sf::ClientProcessId &client_pid, u64 nrr_address) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::UnloadNrr(this->context_id, nrr_address);
    }

    Result Service::Initialize(const sf::ClientProcessId &client_pid, sf::CopyHandle process_h) {
        return impl::RegisterProcess(&this->context_id, process_h.GetValue(), client_pid.GetValue());
    }

    Result Service::LoadNrrEx(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size, sf::CopyHandle process_h) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::LoadNrr(this->context_id, process_h.GetValue(), nrr_address, nrr_size, this->type, this->type == ModuleType::ForOthers);
    }

}
