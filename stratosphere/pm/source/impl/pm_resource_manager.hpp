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
#include <stratosphere.hpp>

namespace ams::pm::resource {

    /* Resource API. */
    Result InitializeResourceManager();
    Result BoostSystemMemoryResourceLimit(u64 boost_size);
    Result BoostApplicationThreadResourceLimit();
    Handle GetResourceLimitHandle(ResourceLimitGroup group);
    Handle GetResourceLimitHandle(const ldr::ProgramInfo *info);
    void   WaitResourceAvailable(const ldr::ProgramInfo *info);

    Result GetResourceLimitValues(u64 *out_cur, u64 *out_lim, ResourceLimitGroup group, LimitableResource resource);

}
