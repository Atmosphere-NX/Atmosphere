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
#ifndef __FS_1900_H__
#define __FS_1900_H__

// Accessor vtable getters
#define FS_OFFSET_1900_SDMMC_ACCESSOR_GC   0x195C00
#define FS_OFFSET_1900_SDMMC_ACCESSOR_SD   0x197F80
#define FS_OFFSET_1900_SDMMC_ACCESSOR_NAND 0x1963B0

// Hooks
#define FS_OFFSET_1900_SDMMC_WRAPPER_READ  0x191A70
#define FS_OFFSET_1900_SDMMC_WRAPPER_WRITE 0x191AD0
#define FS_OFFSET_1900_RTLD                0x275F0
#define FS_OFFSET_1900_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x50)))

#define FS_OFFSET_1900_CLKRST_SET_MIN_V_CLK_RATE 0x1B3880

// Misc funcs
#define FS_OFFSET_1900_LOCK_MUTEX          0x18AC20
#define FS_OFFSET_1900_UNLOCK_MUTEX        0x18AC70

#define FS_OFFSET_1900_SDMMC_WRAPPER_CONTROLLER_OPEN  0x191A30
#define FS_OFFSET_1900_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x191A50

// Misc Data
#define FS_OFFSET_1900_SD_MUTEX            0xFE1408
#define FS_OFFSET_1900_NAND_MUTEX          0xFDCB60
#define FS_OFFSET_1900_ACTIVE_PARTITION    0xFDCBA0
#define FS_OFFSET_1900_SDMMC_DAS_HANDLE    0xFC1908

// NOPs
#define FS_OFFSET_1900_SD_DAS_INIT         0x260C4

// Nintendo Paths
#define FS_OFFSET_1900_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00067FC8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00075D6C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007D1E8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00092818, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1900_H__
