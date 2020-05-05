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

#define AHB_ARBC(x) (0x6000c000 + x)

#define AHB_ARBITRATION_DISABLE       (0x004)
#define AHB_ARBITRATION_PRIORITY_CTRL (0x008)
#define AHB_MASTER_SWID               (0x018)
#define AHB_MASTER_SWID_1             (0x038)
#define AHB_GIZMO_TZRAM               (0x054)

