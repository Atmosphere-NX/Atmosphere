/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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
#ifndef __FS_1000_H__
#define __FS_1000_H__

// Accessor vtable getters
#define FS_OFFSET_1000_SDMMC_ACCESSOR_GC   0x14DC90
#define FS_OFFSET_1000_SDMMC_ACCESSOR_SD   0x14BDA0
#define FS_OFFSET_1000_SDMMC_ACCESSOR_NAND 0x146C20

// Hooks
#define FS_OFFSET_1000_SDMMC_WRAPPER_READ  0x142380
#define FS_OFFSET_1000_SDMMC_WRAPPER_WRITE 0x142460
#define FS_OFFSET_1000_RTLD                0x634
#define FS_OFFSET_1000_RTLD_DESTINATION    0x9C

#define FS_OFFSET_1000_CLKRST_SET_MIN_V_CLK_RATE 0x1415A0

// Misc funcs
#define FS_OFFSET_1000_LOCK_MUTEX          0x28910
#define FS_OFFSET_1000_UNLOCK_MUTEX        0x28960

#define FS_OFFSET_1000_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_1000_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1422E0

// Misc Data
#define FS_OFFSET_1000_SD_MUTEX            0xE273E8
#define FS_OFFSET_1000_NAND_MUTEX          0xE22DA0
#define FS_OFFSET_1000_ACTIVE_PARTITION    0xE22DE0
#define FS_OFFSET_1000_SDMMC_DAS_HANDLE    0xE0AB90

// NOPs
#define FS_OFFSET_1000_SD_DAS_INIT         0x151CEC

// Nintendo Paths
#define FS_OFFSET_1000_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006BBA4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00078520, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007ED0C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0009115C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1000_H__
