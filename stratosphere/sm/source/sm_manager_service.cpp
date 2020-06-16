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
#include "sm_manager_service.hpp"
#include "impl/sm_service_manager.hpp"

namespace ams::sm {

    Result ManagerService::RegisterProcess(os::ProcessId process_id, const sf::InBuffer &acid_sac, const sf::InBuffer &aci_sac) {
        return impl::RegisterProcess(process_id, ncm::InvalidProgramId, cfg::OverrideStatus{}, acid_sac.GetPointer(), acid_sac.GetSize(), aci_sac.GetPointer(), aci_sac.GetSize());
    }

    Result ManagerService::UnregisterProcess(os::ProcessId process_id) {
        return impl::UnregisterProcess(process_id);
    }

    void ManagerService::AtmosphereEndInitDefers() {
        R_ABORT_UNLESS(impl::EndInitialDefers());
    }

    void ManagerService::AtmosphereHasMitm(sf::Out<bool> out, ServiceName service) {
        R_ABORT_UNLESS(impl::HasMitm(out.GetPointer(), service));
    }

    Result ManagerService::AtmosphereRegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const sf::InBuffer &acid_sac, const sf::InBuffer &aci_sac) {
        /* This takes in a program id and override status, unlike RegisterProcess. */
        return impl::RegisterProcess(process_id, program_id, override_status, acid_sac.GetPointer(), acid_sac.GetSize(), aci_sac.GetPointer(), aci_sac.GetSize());
    }

}
