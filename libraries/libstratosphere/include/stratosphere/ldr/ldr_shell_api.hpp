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

#pragma once
#include <vapours.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/ldr/ldr_types.hpp>

namespace ams::ldr {

    /* Shell API. */
    Result InitializeForShell();
    Result FinalizeForShell();

    Result SetProgramArgument(ncm::ProgramId program_id, const void *arg, size_t size);
    Result FlushArguments();

}
