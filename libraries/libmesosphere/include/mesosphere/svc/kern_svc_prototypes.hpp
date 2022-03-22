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
#include <vapours.hpp>
#include <mesosphere/svc/kern_svc_k_user_pointer.hpp>
#include <mesosphere/svc/kern_svc_results.hpp>

namespace ams::kern::svc {

    static constexpr size_t NumSupervisorCalls = AMS_KERN_NUM_SUPERVISOR_CALLS;

    #define AMS_KERN_SVC_DECLARE_ENUM_ID(ID, RETURN_TYPE, NAME, ...) \
        SvcId_##NAME = ID,

    enum SvcId {
        AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_DECLARE_ENUM_ID, __invalid)
        SvcId_Count = NumSupervisorCalls,
    };

    #undef AMS_KERN_SVC_DECLARE_ENUM_ID

    #define AMS_KERN_SVC_DECLARE_PROTOTYPE_64(ID, RETURN_TYPE, NAME, ...) \
        NOINLINE RETURN_TYPE NAME##64(__VA_ARGS__);
    #define AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32(ID, RETURN_TYPE, NAME, ...) \
        NOINLINE RETURN_TYPE NAME##64From32(__VA_ARGS__);

    AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_DECLARE_PROTOTYPE_64,         lp64)
    AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32, ilp32)

    /* TODO: Support _32 ABI */

    #undef AMS_KERN_SVC_DECLARE_PROTOTYPE_64
    #undef AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32

    struct SvcAccessFlagSetTag{};

    using SvcAccessFlagSet = util::BitFlagSet<NumSupervisorCalls, SvcAccessFlagSetTag>;

}
