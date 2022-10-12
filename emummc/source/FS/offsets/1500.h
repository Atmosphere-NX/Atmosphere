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
#ifndef __FS_1500_H__
#define __FS_1500_H__

// Accessor vtable getters
#define FS_OFFSET_1500_SDMMC_ACCESSOR_GC   0x183E20
#define FS_OFFSET_1500_SDMMC_ACCESSOR_SD   0x185AA0
#define FS_OFFSET_1500_SDMMC_ACCESSOR_NAND 0x1842E0

// Hooks
#define FS_OFFSET_1500_SDMMC_WRAPPER_READ  0x17FCF0
#define FS_OFFSET_1500_SDMMC_WRAPPER_WRITE 0x17FD50
#define FS_OFFSET_1500_RTLD                0x26518
#define FS_OFFSET_1500_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1500_CLKRST_SET_MIN_V_CLK_RATE 0x1A0870

// Misc funcs
#define FS_OFFSET_1500_LOCK_MUTEX          0x1791A0
#define FS_OFFSET_1500_UNLOCK_MUTEX        0x1791F0

#define FS_OFFSET_1500_SDMMC_WRAPPER_CONTROLLER_OPEN  0x17FCB0
#define FS_OFFSET_1500_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x17FCD0

// Misc Data
#define FS_OFFSET_1500_SD_MUTEX            0xFF33F0
#define FS_OFFSET_1500_NAND_MUTEX          0xFEE2E8
#define FS_OFFSET_1500_ACTIVE_PARTITION    0xFEE328
#define FS_OFFSET_1500_SDMMC_DAS_HANDLE    0xFD38D8

// NOPs
#define FS_OFFSET_1500_SD_DAS_INIT         0x25454

// Nintendo Paths
#define FS_OFFSET_1500_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00063050, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0006FDE8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x000768D4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00089364, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1500_H__
