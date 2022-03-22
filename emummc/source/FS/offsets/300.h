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
#ifndef __FS_300_H__
#define __FS_300_H__

// Accessor vtable getters
#define FS_OFFSET_300_SDMMC_ACCESSOR_GC   0x8FAAC
#define FS_OFFSET_300_SDMMC_ACCESSOR_SD   0x8F84C
#define FS_OFFSET_300_SDMMC_ACCESSOR_NAND 0x8F3B8

// Hooks
#define FS_OFFSET_300_SDMMC_WRAPPER_READ  0x8A2F4
#define FS_OFFSET_300_SDMMC_WRAPPER_WRITE 0x8A3B4
#define FS_OFFSET_300_RTLD                0x4E8
#define FS_OFFSET_300_RTLD_DESTINATION    0x8C

#define FS_OFFSET_300_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_300_LOCK_MUTEX          0x35CC
#define FS_OFFSET_300_UNLOCK_MUTEX        0x3638

#define FS_OFFSET_300_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_300_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x8A270

// Misc Data
#define FS_OFFSET_300_SD_MUTEX            0xE69268
#define FS_OFFSET_300_NAND_MUTEX          0xE646F0
#define FS_OFFSET_300_ACTIVE_PARTITION    0xE64730
#define FS_OFFSET_300_SDMMC_DAS_HANDLE    0xE635A0

// NOPs
#define FS_OFFSET_300_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_300_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x000391F4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003A480, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003A778, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_300_H__
