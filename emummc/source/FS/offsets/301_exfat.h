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
#ifndef __FS_301_EXFAT_H__
#define __FS_301_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_301_EXFAT_SDMMC_ACCESSOR_GC   0x8FB68
#define FS_OFFSET_301_EXFAT_SDMMC_ACCESSOR_SD   0x8F908
#define FS_OFFSET_301_EXFAT_SDMMC_ACCESSOR_NAND 0x8F474

// Hooks
#define FS_OFFSET_301_EXFAT_SDMMC_WRAPPER_READ  0x8A3B0
#define FS_OFFSET_301_EXFAT_SDMMC_WRAPPER_WRITE 0x8A470
#define FS_OFFSET_301_EXFAT_RTLD                0x4E8
#define FS_OFFSET_301_EXFAT_RTLD_DESTINATION    0x8C

#define FS_OFFSET_301_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_301_EXFAT_LOCK_MUTEX          0x3638
#define FS_OFFSET_301_EXFAT_UNLOCK_MUTEX        0x36A4

#define FS_OFFSET_301_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_301_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x8A32C

// Misc Data
#define FS_OFFSET_301_EXFAT_SD_MUTEX            0xF4C268
#define FS_OFFSET_301_EXFAT_NAND_MUTEX          0xF476F0
#define FS_OFFSET_301_EXFAT_ACTIVE_PARTITION    0xF47730
#define FS_OFFSET_301_EXFAT_SDMMC_DAS_HANDLE    0xF465A0

// NOPs
#define FS_OFFSET_301_EXFAT_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_301_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00039260, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003A4EC, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 3, .adrp_offset = 0x0003A7E4, .add_rel_offset = 0x0000000C}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_301_EXFAT_H__
