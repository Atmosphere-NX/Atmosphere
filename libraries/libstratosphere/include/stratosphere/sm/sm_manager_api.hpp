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
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/cfg/cfg_types.hpp>
#include <stratosphere/sm/sm_types.hpp>

namespace ams::sm::manager {

    /* Manager API. */
    Result RegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus status, const void *acid, size_t acid_size, const void *aci, size_t aci_size);
    Result UnregisterProcess(os::ProcessId process_id);

    /* Atmosphere extensions. */
    Result EndInitialDefers();
    Result HasMitm(bool *out, ServiceName name);

}
