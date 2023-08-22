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
#if defined(ATMOSPHERE_OS_HORIZON)
#include "mitm_pm.os.horizon.h"
#endif

namespace ams::mitm::pm {

    /* PM API. */
    #if defined(ATMOSPHERE_OS_HORIZON)
    void Initialize() {
        R_ABORT_UNLESS(amsMitmPmInitialize());
    }

    void Finalize() {
        amsMitmPmExit();
    }

    Result PrepareLaunchProgram(u64 *out, ncm::ProgramId program_id, const cfg::OverrideStatus &status, bool is_application) {
        static_assert(sizeof(status) == sizeof(CfgOverrideStatus), "CfgOverrideStatus definition!");
        R_RETURN(amsMitmPmPrepareLaunchProgram(out, program_id.value, reinterpret_cast<const CfgOverrideStatus *>(std::addressof(status)), is_application));
    }
    #endif

}
