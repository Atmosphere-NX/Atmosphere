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
#include "ro_ro_service.hpp"
#include "impl/ro_service_impl.hpp"

namespace ams::ro {

    void SetDevelopmentHardware(bool is_development_hardware) {
        impl::SetDevelopmentHardware(is_development_hardware);
    }

    void SetDevelopmentFunctionEnabled(bool is_development_function_enabled) {
        impl::SetDevelopmentFunctionEnabled(is_development_function_enabled);
    }

    RoService::RoService(NrrKind k) : context_id(impl::InvalidContextId), nrr_kind(k) {
        /* ... */
    }

    RoService::~RoService() {
        impl::UnregisterProcess(this->context_id);
    }

    Result RoService::MapManualLoadModuleMemory(sf::Out<u64> load_address, const sf::ClientProcessId &client_pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::MapManualLoadModuleMemory(load_address.GetPointer(), this->context_id, nro_address, nro_size, bss_address, bss_size);
    }

    Result RoService::UnmapManualLoadModuleMemory(const sf::ClientProcessId &client_pid, u64 nro_address) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::UnmapManualLoadModuleMemory(this->context_id, nro_address);
    }

    Result RoService::RegisterModuleInfo(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::RegisterModuleInfo(this->context_id, svc::InvalidHandle, nrr_address, nrr_size, NrrKind_User, true);
    }

    Result RoService::UnregisterModuleInfo(const sf::ClientProcessId &client_pid, u64 nrr_address) {
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::UnregisterModuleInfo(this->context_id, nrr_address);
    }

    Result RoService::RegisterProcessHandle(const sf::ClientProcessId &client_pid, sf::CopyHandle process_h) {
        /* Ensure we manage references to the process handle correctly. */
        os::ManagedHandle process_handle(process_h.GetValue());

        /* Register the process. */
        return impl::RegisterProcess(std::addressof(this->context_id), std::move(process_handle), client_pid.GetValue());
    }

    Result RoService::RegisterProcessModuleInfo(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size, sf::CopyHandle process_h) {
        /* Ensure we manage references to the process handle correctly. */
        os::ManagedHandle process_handle(process_h.GetValue());

        /* Register the module. */
        R_TRY(impl::ValidateProcess(this->context_id, client_pid.GetValue()));
        return impl::RegisterModuleInfo(this->context_id, std::move(process_handle), nrr_address, nrr_size, this->nrr_kind, this->nrr_kind == NrrKind_JitPlugin);
    }

}
