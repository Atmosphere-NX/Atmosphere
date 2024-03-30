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
#include "impl/fssrv_program_info.hpp"

namespace ams::fssrv {

    namespace {

        constinit ProgramRegistryServiceImpl *g_impl = nullptr;

    }

    ProgramRegistryImpl::ProgramRegistryImpl() : m_process_id(os::InvalidProcessId.value) {
        /* ... */
    }

    ProgramRegistryImpl::~ProgramRegistryImpl() {
        /* ... */
    }

    void ProgramRegistryImpl::Initialize(ProgramRegistryServiceImpl *service) {
        /* Check pre-conditions. */
        AMS_ASSERT(service != nullptr);
        AMS_ASSERT(g_impl  == nullptr);

        /* Set the global service. */
        g_impl = service;
    }

    Result ProgramRegistryImpl::RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const ams::sf::InBuffer &data, s64 data_size, const ams::sf::InBuffer &desc, s64 desc_size) {
        /* Check pre-conditions. */
        AMS_ASSERT(g_impl != nullptr);

        /* Check that we're allowed to register. */
        R_UNLESS(fssrv::impl::IsInitialProgram(m_process_id), fs::ResultPermissionDenied());

        /* Check buffer sizes. */
        R_UNLESS(data.GetSize() >= static_cast<size_t>(data_size), fs::ResultInvalidSize());
        R_UNLESS(desc.GetSize() >= static_cast<size_t>(desc_size), fs::ResultInvalidSize());

        /* Register the program. */
        R_RETURN(g_impl->RegisterProgramInfo(process_id, program_id, storage_id, data.GetPointer(), data_size, desc.GetPointer(), desc_size));
    }

    Result ProgramRegistryImpl::UnregisterProgram(u64 process_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(g_impl != nullptr);

        /* Check that we're allowed to register. */
        R_UNLESS(fssrv::impl::IsInitialProgram(m_process_id), fs::ResultPermissionDenied());

        /* Unregister the program. */
        R_RETURN(g_impl->UnregisterProgramInfo(process_id));
    }

    Result ProgramRegistryImpl::SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
        /* Set our process id. */
        m_process_id = client_pid.GetValue().value;

        R_SUCCEED();
    }

    Result ProgramRegistryImpl::SetEnabledProgramVerification(bool en) {
        /* TODO: How to deal with this backwards compat? */
        AMS_ABORT("TODO: SetEnabledProgramVerification");
        AMS_UNUSED(en);
    }

}
