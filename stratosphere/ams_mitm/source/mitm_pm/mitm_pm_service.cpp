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

namespace ams::mitm::pm {

    Result PmService::PrepareLaunchProgram(sf::Out<u64> out, ncm::ProgramId program_id, const cfg::OverrideStatus &status, bool is_application) {
        /* TODO */
        *out = 0;
        AMS_UNUSED(program_id, status, is_application);
        R_SUCCEED();
    }

}
