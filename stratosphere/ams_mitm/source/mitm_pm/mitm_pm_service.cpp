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
#include "../amsmitm_initialization.hpp"
#include "mitm_pm_service.hpp"
#include "mitm_pm_service.hpp"
#include "../fs_mitm/fsmitm_romfs.hpp"

namespace ams::mitm::pm {

    Result PmService::PrepareLaunchProgram(sf::Out<u64> out, ncm::ProgramId program_id, const cfg::OverrideStatus &status, bool is_application) {
        /* Default to zero heap. */
        *out = 0;

        /* Actually configure the required boost size for romfs. */
        R_TRY(mitm::fs::romfs::ConfigureDynamicHeap(out.GetPointer(), program_id, status, is_application));

        /* TODO: Is there anything else we should do, while we have the opportunity? */
        R_SUCCEED();
    }

}
