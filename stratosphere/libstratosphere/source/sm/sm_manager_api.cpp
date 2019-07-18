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
#include <stratosphere/sm.hpp>
#include <stratosphere/sm/sm_manager_api.hpp>

#include "smm_ams.h"

namespace sts::sm::manager {

    /* Manager API. */
    Result RegisterProcess(u64 process_id, ncm::TitleId title_id, const void *acid, size_t acid_size, const void *aci, size_t aci_size) {
        return smManagerAtmosphereRegisterProcess(process_id, static_cast<u64>(title_id), acid, acid_size, aci, aci_size);
    }

    Result UnregisterProcess(u64 process_id) {
        return smManagerUnregisterProcess(process_id);
    }

    /* Atmosphere extensions. */
    Result EndInitialDefers() {
        return smManagerAtmosphereEndInitialDefers();
    }

    Result HasMitm(bool *out, ServiceName name) {
        return smManagerAtmosphereHasMitm(out, name.name);
    }

}
