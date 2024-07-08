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
#ifndef __FS_1810_H__
#define __FS_1810_H__

// Accessor vtable getters
#define FS_OFFSET_1810_SDMMC_ACCESSOR_GC   0x18AB00
#define FS_OFFSET_1810_SDMMC_ACCESSOR_SD   0x18C800
#define FS_OFFSET_1810_SDMMC_ACCESSOR_NAND 0x18AFE0

// Hooks
#define FS_OFFSET_1810_SDMMC_WRAPPER_READ  0x186A50
#define FS_OFFSET_1810_SDMMC_WRAPPER_WRITE 0x186AB0
#define FS_OFFSET_1810_RTLD                0x2A3A4
#define FS_OFFSET_1810_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x44)))

#define FS_OFFSET_1810_CLKRST_SET_MIN_V_CLK_RATE 0x1A77D0

// Misc funcs
#define FS_OFFSET_1810_LOCK_MUTEX          0x17FCC0
#define FS_OFFSET_1810_UNLOCK_MUTEX        0x17FD10

#define FS_OFFSET_1810_SDMMC_WRAPPER_CONTROLLER_OPEN  0x186A10
#define FS_OFFSET_1810_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x186A30

// Misc Data
#define FS_OFFSET_1810_SD_MUTEX            0xFD13F0
#define FS_OFFSET_1810_NAND_MUTEX          0xFCCB28
#define FS_OFFSET_1810_ACTIVE_PARTITION    0xFCCB68
#define FS_OFFSET_1810_SDMMC_DAS_HANDLE    0xFB1950

// NOPs
#define FS_OFFSET_1810_SD_DAS_INIT         0x28F24

// Nintendo Paths
#define FS_OFFSET_1810_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00068B08, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x000758DC, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007C77C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x000905C4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1810_H__
