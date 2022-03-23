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
#ifndef __FS_910_EXFAT_H__
#define __FS_910_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_910_EXFAT_SDMMC_ACCESSOR_GC   0x143100
#define FS_OFFSET_910_EXFAT_SDMMC_ACCESSOR_SD   0x141210
#define FS_OFFSET_910_EXFAT_SDMMC_ACCESSOR_NAND 0x13C090

// Hooks
#define FS_OFFSET_910_EXFAT_SDMMC_WRAPPER_READ  0x1377F0
#define FS_OFFSET_910_EXFAT_SDMMC_WRAPPER_WRITE 0x1378D0
#define FS_OFFSET_910_EXFAT_RTLD                0x454
#define FS_OFFSET_910_EXFAT_RTLD_DESTINATION    0x9C

#define FS_OFFSET_910_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x136A10

// Misc funcs
#define FS_OFFSET_910_EXFAT_LOCK_MUTEX          0x25280
#define FS_OFFSET_910_EXFAT_UNLOCK_MUTEX        0x252D0

#define FS_OFFSET_910_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_910_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x137750

// Misc Data
#define FS_OFFSET_910_EXFAT_SD_MUTEX            0xE2B3E8
#define FS_OFFSET_910_EXFAT_NAND_MUTEX          0xE26258
#define FS_OFFSET_910_EXFAT_ACTIVE_PARTITION    0xE26298
#define FS_OFFSET_910_EXFAT_SDMMC_DAS_HANDLE    0xE0CFA0

// NOPs
#define FS_OFFSET_910_EXFAT_SD_DAS_INIT         0x1472CC

// Nintendo Paths
#define FS_OFFSET_910_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00068A70, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00070A50, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00081CC4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00081F04, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008212C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_910_EXFAT_H__
