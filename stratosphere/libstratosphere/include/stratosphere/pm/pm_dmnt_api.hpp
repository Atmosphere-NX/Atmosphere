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

#pragma once

#include "../ldr.hpp"
#include "pm_types.hpp"

namespace sts::pm::dmnt {

    /* Debug Monitor API. */
    Result StartProcess(u64 process_id);
    Result GetProcessId(u64 *out_process_id, const ncm::TitleId title_id);
    Result GetApplicationProcessId(u64 *out_process_id);
    Result HookToCreateApplicationProcess(Handle *out_handle);
    Result AtmosphereGetProcessInfo(Handle *out_handle, ncm::TitleLocation *out_loc, u64 process_id);
    Result AtmosphereGetCurrentLimitInfo(u64 *out_current_value, u64 *out_limit_value, ResourceLimitGroup group, LimitableResource resource);

}
