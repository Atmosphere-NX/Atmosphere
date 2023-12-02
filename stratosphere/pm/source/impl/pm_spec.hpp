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
#include <stratosphere.hpp>

namespace ams::pm::impl {

    Result InitializeSpec();

    Result BoostSystemMemoryResourceLimit(u64 boost_size);
    Result BoostApplicationThreadResourceLimit();
    Result BoostSystemThreadResourceLimit();

    Result BoostSystemMemoryResourceLimitForMitm(u64 boost_size);

    os::NativeHandle GetResourceLimitHandle(ResourceLimitGroup group);
    os::NativeHandle GetResourceLimitHandle(const ldr::ProgramInfo *info);

    void WaitResourceAvailable(const ldr::ProgramInfo *info);

    Result GetResourceLimitCurrentValue(pm::ResourceLimitValue *out, ResourceLimitGroup group);
    Result GetResourceLimitPeakValue(pm::ResourceLimitValue *outm, ResourceLimitGroup group);
    Result GetResourceLimitLimitValue(pm::ResourceLimitValue *out, ResourceLimitGroup group);

    Result GetResourceLimitValues(s64 *out_cur, s64 *out_lim, ResourceLimitGroup group, svc::LimitableResource resource);

}
