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
#include "pm_ams.h"

namespace ams::pm::info {

    /* Information API. */
    Result WEAK_SYMBOL HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id) {
        bool has_launched = false;
        R_TRY(pminfoAtmosphereHasLaunchedBootProgram(std::addressof(has_launched), static_cast<u64>(program_id)));
        *out = has_launched;
        return ResultSuccess();
    }

}
