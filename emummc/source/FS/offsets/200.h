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
#ifndef __FS_200_H__
#define __FS_200_H__

// Accessor vtable getters
#define FS_OFFSET_200_SDMMC_ACCESSOR_GC   0x78BAC
#define FS_OFFSET_200_SDMMC_ACCESSOR_SD   0x7894C
#define FS_OFFSET_200_SDMMC_ACCESSOR_NAND 0x784BC

// Hooks
#define FS_OFFSET_200_SDMMC_WRAPPER_READ  0x73478
#define FS_OFFSET_200_SDMMC_WRAPPER_WRITE 0x73538
#define FS_OFFSET_200_RTLD                0x500
#define FS_OFFSET_200_RTLD_DESTINATION    0x98

#define FS_OFFSET_200_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_200_LOCK_MUTEX          0x3264
#define FS_OFFSET_200_UNLOCK_MUTEX        0x32D0

#define FS_OFFSET_200_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_200_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x733F4

// Misc Data
#define FS_OFFSET_200_SD_MUTEX            0xE42268
#define FS_OFFSET_200_NAND_MUTEX          0xE3CED0
#define FS_OFFSET_200_ACTIVE_PARTITION    0xE3CF10
#define FS_OFFSET_200_SDMMC_DAS_HANDLE    0xE3BDD0

// NOPs
#define FS_OFFSET_200_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_200_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00033F08, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00035084, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003537C, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_200_H__
