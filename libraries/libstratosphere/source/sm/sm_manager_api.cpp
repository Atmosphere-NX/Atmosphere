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
#include "sm_utils.hpp"
#include "smm_ams.h"

namespace ams::sm::manager {

    /* Manager API. */
    Result RegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus status, const void *acid, size_t acid_size, const void *aci, size_t aci_size) {
        static_assert(sizeof(status) == sizeof(CfgOverrideStatus), "CfgOverrideStatus definition");
        return smManagerAtmosphereRegisterProcess(static_cast<u64>(process_id), static_cast<u64>(program_id), reinterpret_cast<const CfgOverrideStatus *>(&status), acid, acid_size, aci, aci_size);
    }

    Result UnregisterProcess(os::ProcessId process_id) {
        return smManagerUnregisterProcess(static_cast<u64>(process_id));
    }

    /* Atmosphere extensions. */
    Result EndInitialDefers() {
        return smManagerAtmosphereEndInitialDefers();
    }

    Result HasMitm(bool *out, ServiceName name) {
        return smManagerAtmosphereHasMitm(out, impl::ConvertName(name));
    }

}
