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
#ifndef __FS_2100_H__
#define __FS_2100_H__

// Accessor vtable getters
#define FS_OFFSET_2100_SDMMC_ACCESSOR_GC   0x1AC970
#define FS_OFFSET_2100_SDMMC_ACCESSOR_SD   0x1AE980
#define FS_OFFSET_2100_SDMMC_ACCESSOR_NAND 0x1ACFA0

// Hooks
#define FS_OFFSET_2100_SDMMC_WRAPPER_READ  0x1A8850
#define FS_OFFSET_2100_SDMMC_WRAPPER_WRITE 0x1A88B0
#define FS_OFFSET_2100_RTLD                0x2E1C0
#define FS_OFFSET_2100_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2100_CLKRST_SET_MIN_V_CLK_RATE 0x1CB9B0

// Misc funcs
#define FS_OFFSET_2100_LOCK_MUTEX          0x1A17D0
#define FS_OFFSET_2100_UNLOCK_MUTEX        0x1A1830

#define FS_OFFSET_2100_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1A8810
#define FS_OFFSET_2100_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1A8830

// Misc Data
#define FS_OFFSET_2100_SD_MUTEX            0xFEE408
#define FS_OFFSET_2100_NAND_MUTEX          0xFE9CF0
#define FS_OFFSET_2100_ACTIVE_PARTITION    0xFE9D30
#define FS_OFFSET_2100_SDMMC_DAS_HANDLE    0xFCBB18

// NOPs
#define FS_OFFSET_2100_SD_DAS_INIT         0x2B5C8

// Nintendo Paths
#define FS_OFFSET_2100_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x000718CC, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x000824F4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008AF18, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x000A0B8C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2100_H__
