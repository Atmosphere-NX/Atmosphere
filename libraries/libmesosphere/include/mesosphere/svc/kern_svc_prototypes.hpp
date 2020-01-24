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
#include "kern_svc_k_user_pointer.hpp"

namespace ams::kern::svc {

    #define AMS_KERN_SVC_DECLARE_PROTOTYPE_64(ID, RETURN_TYPE, NAME, ...) \
        RETURN_TYPE NAME##64(__VA_ARGS__);
    #define AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32(ID, RETURN_TYPE, NAME, ...) \
        RETURN_TYPE NAME##64From32(__VA_ARGS__);

    AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_DECLARE_PROTOTYPE_64,         lp64)
    AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32, ilp32)

    /* TODO: Support _32 ABI */

    #undef AMS_KERN_SVC_DECLARE_PROTOTYPE_64
    #undef AMS_KERN_SVC_DECLARE_PROTOTYPE_64_FROM_32


}
