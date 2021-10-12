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
    class ManagerService {
        public:
            Result RegisterProcess(os::ProcessId process_id, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac) {
                return impl::RegisterProcess(process_id, ncm::InvalidProgramId, cfg::OverrideStatus{}, acid_sac.GetPointer(), acid_sac.GetSize(), aci_sac.GetPointer(), aci_sac.GetSize());
            }

            Result UnregisterProcess(os::ProcessId process_id) {
                return impl::UnregisterProcess(process_id);
            }

            void AtmosphereEndInitDefers() {
                R_ABORT_UNLESS(impl::EndInitialDefers());
            }

            void AtmosphereHasMitm(tipc::Out<bool> out, ServiceName service) {
                R_ABORT_UNLESS(impl::HasMitm(out.GetPointer(), service));
            }

            Result AtmosphereRegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const tipc::InBuffer acid_sac, const tipc::InBuffer aci_sac) {
                /* This takes in a program id and override status, unlike RegisterProcess. */
                return impl::RegisterProcess(process_id, program_id, override_status, acid_sac.GetPointer(), acid_sac.GetSize(), aci_sac.GetPointer(), aci_sac.GetSize());
            }
    };
    static_assert(sm::impl::IsIManagerInterface<ManagerService>);

}
