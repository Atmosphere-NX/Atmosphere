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
#include <mesosphere/kern_build_config.hpp>
#include <mesosphere/svc/kern_svc_results.hpp>
#include <mesosphere/kern_select_assembly_offsets.h>

namespace ams::kern {

    constexpr size_t PageSize = 4_KB;

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    ams::TargetFirmware GetTargetFirmware();
#else
    consteval ALWAYS_INLINE ams::TargetFirmware GetTargetFirmware() {
        return ams::TargetFirmware_Current;
    }
#endif

}
