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
#ifndef __FS_2010_H__
#define __FS_2010_H__

// Accessor vtable getters
#define FS_OFFSET_2010_SDMMC_ACCESSOR_GC   0x1A7DB0
#define FS_OFFSET_2010_SDMMC_ACCESSOR_SD   0x1AA130
#define FS_OFFSET_2010_SDMMC_ACCESSOR_NAND 0x1A8560

// Hooks
#define FS_OFFSET_2010_SDMMC_WRAPPER_READ  0x1A3C20
#define FS_OFFSET_2010_SDMMC_WRAPPER_WRITE 0x1A3C80
#define FS_OFFSET_2010_RTLD                0x2B594
#define FS_OFFSET_2010_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2010_CLKRST_SET_MIN_V_CLK_RATE 0x1C6150

// Misc funcs
#define FS_OFFSET_2010_LOCK_MUTEX          0x19CD80
#define FS_OFFSET_2010_UNLOCK_MUTEX        0x19CDD0

#define FS_OFFSET_2010_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1A3BE0
#define FS_OFFSET_2010_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1A3C00

// Misc Data
#define FS_OFFSET_2010_SD_MUTEX            0xFF5408
#define FS_OFFSET_2010_NAND_MUTEX          0xFF0CF0
#define FS_OFFSET_2010_ACTIVE_PARTITION    0xFF0D30
#define FS_OFFSET_2010_SDMMC_DAS_HANDLE    0xFD2B08

// NOPs
#define FS_OFFSET_2010_SD_DAS_INIT         0x289F4

// Nintendo Paths
#define FS_OFFSET_2010_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006DB14, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007CE1C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00084A08, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0009AE48, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2010_H__
