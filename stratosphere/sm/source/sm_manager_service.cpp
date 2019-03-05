/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include "sm_registration.hpp"

Result ManagerService::RegisterProcess(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac) {
    return Registration::RegisterProcess(pid, acid_sac.buffer, acid_sac.num_elements, aci0_sac.buffer, aci0_sac.num_elements);
}

Result ManagerService::UnregisterProcess(u64 pid) {
    return Registration::UnregisterProcess(pid);
}

void ManagerService::AtmosphereEndInitDefers() {
    Registration::EndInitDefers();
}

void ManagerService::AtmosphereHasMitm(Out<bool> out, SmServiceName service) {
    out.SetValue(Registration::HasMitm(smEncodeName(service.name)));
}

