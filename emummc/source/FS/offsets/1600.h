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
#ifndef __FS_1600_H__
#define __FS_1600_H__

// Accessor vtable getters
#define FS_OFFSET_1600_SDMMC_ACCESSOR_GC   0x1862A0
#define FS_OFFSET_1600_SDMMC_ACCESSOR_SD   0x187F20
#define FS_OFFSET_1600_SDMMC_ACCESSOR_NAND 0x186760

// Hooks
#define FS_OFFSET_1600_SDMMC_WRAPPER_READ  0x1821F0
#define FS_OFFSET_1600_SDMMC_WRAPPER_WRITE 0x182250
#define FS_OFFSET_1600_RTLD                0x269B0
#define FS_OFFSET_1600_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1600_CLKRST_SET_MIN_V_CLK_RATE 0x1A2D30

// Misc funcs
#define FS_OFFSET_1600_LOCK_MUTEX          0x17B730
#define FS_OFFSET_1600_UNLOCK_MUTEX        0x17B780

#define FS_OFFSET_1600_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1821B0
#define FS_OFFSET_1600_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1821D0

// Misc Data
#define FS_OFFSET_1600_SD_MUTEX            0xFFB3F0
#define FS_OFFSET_1600_NAND_MUTEX          0xFF6B58
#define FS_OFFSET_1600_ACTIVE_PARTITION    0xFF6B98
#define FS_OFFSET_1600_SDMMC_DAS_HANDLE    0xFDC8B0

// NOPs
#define FS_OFFSET_1600_SD_DAS_INIT         0x258D4

// Nintendo Paths
#define FS_OFFSET_1600_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00063B48, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00070D6C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007790C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008A754, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1600_H__
