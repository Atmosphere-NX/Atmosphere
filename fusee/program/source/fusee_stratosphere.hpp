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
#include <vapours.hpp>
#pragma once

namespace ams::nxboot {

    u32 ConfigureStratosphere(const u8 *nn_package2, ams::TargetFirmware target_firmware, bool emummc_enabled, bool nogc_enabled);

    void RebuildPackage2(ams::TargetFirmware target_firmware, bool emummc_enabled);

}