/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>

#include "sm_manager_service.hpp"
#include "impl/sm_service_manager.hpp"

namespace sts::sm {

    Result ManagerService::RegisterProcess(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci_sac) {
        return impl::RegisterProcess(pid, ncm::TitleId::Invalid, acid_sac.buffer, acid_sac.num_elements, aci_sac.buffer, aci_sac.num_elements);
    }

    Result ManagerService::UnregisterProcess(u64 pid) {
        return impl::UnregisterProcess(pid);
    }

    void ManagerService::AtmosphereEndInitDefers() {
        R_ASSERT(impl::EndInitialDefers());
    }

    void ManagerService::AtmosphereHasMitm(Out<bool> out, ServiceName service) {
        R_ASSERT(impl::HasMitm(out.GetPointer(), service));
    }

    Result ManagerService::AtmosphereRegisterProcess(u64 pid, ncm::TitleId tid, InBuffer<u8> acid_sac, InBuffer<u8> aci_sac) {
        /* This takes in a title id, unlike RegisterProcess. */
        return impl::RegisterProcess(pid, tid, acid_sac.buffer, acid_sac.num_elements, aci_sac.buffer, aci_sac.num_elements);
    }

}
