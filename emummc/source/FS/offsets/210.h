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
#ifndef __FS_210_H__
#define __FS_210_H__

// Accessor vtable getters
#define FS_OFFSET_210_SDMMC_ACCESSOR_GC   0x78F8C
#define FS_OFFSET_210_SDMMC_ACCESSOR_SD   0x78D2C
#define FS_OFFSET_210_SDMMC_ACCESSOR_NAND 0x7889C

// Hooks
#define FS_OFFSET_210_SDMMC_WRAPPER_READ  0x73858
#define FS_OFFSET_210_SDMMC_WRAPPER_WRITE 0x73918
#define FS_OFFSET_210_RTLD                0x500
#define FS_OFFSET_210_RTLD_DESTINATION    0x98

#define FS_OFFSET_210_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_210_LOCK_MUTEX          0x3264
#define FS_OFFSET_210_UNLOCK_MUTEX        0x32D0

#define FS_OFFSET_210_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_210_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x737D4

// Misc Data
#define FS_OFFSET_210_SD_MUTEX            0xE43268
#define FS_OFFSET_210_NAND_MUTEX          0xE3DED0
#define FS_OFFSET_210_ACTIVE_PARTITION    0xE3DF10
#define FS_OFFSET_210_SDMMC_DAS_HANDLE    0xE3CDD0

// NOPs
#define FS_OFFSET_210_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_210_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x000342E0, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003545C, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 3, .adrp_offset = 0x00035754, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_210_H__
