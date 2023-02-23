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
#ifndef __FS_1600_EXFAT_H__
#define __FS_1600_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1600_EXFAT_SDMMC_ACCESSOR_GC   0x190F80
#define FS_OFFSET_1600_EXFAT_SDMMC_ACCESSOR_SD   0x192C00
#define FS_OFFSET_1600_EXFAT_SDMMC_ACCESSOR_NAND 0x191440

// Hooks
#define FS_OFFSET_1600_EXFAT_SDMMC_WRAPPER_READ  0x18CED0
#define FS_OFFSET_1600_EXFAT_SDMMC_WRAPPER_WRITE 0x18CF30
#define FS_OFFSET_1600_EXFAT_RTLD                0x269B0
#define FS_OFFSET_1600_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1600_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1ADA10

// Misc funcs
#define FS_OFFSET_1600_EXFAT_LOCK_MUTEX          0x186410
#define FS_OFFSET_1600_EXFAT_UNLOCK_MUTEX        0x186460

#define FS_OFFSET_1600_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x18CE90
#define FS_OFFSET_1600_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x18CEB0

// Misc Data
#define FS_OFFSET_1600_EXFAT_SD_MUTEX            0x100D3F0
#define FS_OFFSET_1600_EXFAT_NAND_MUTEX          0x1008B58
#define FS_OFFSET_1600_EXFAT_ACTIVE_PARTITION    0x1008B98
#define FS_OFFSET_1600_EXFAT_SDMMC_DAS_HANDLE    0xFE98B0

// NOPs
#define FS_OFFSET_1600_EXFAT_SD_DAS_INIT         0x258D4

// Nintendo Paths
#define FS_OFFSET_1600_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00063B48, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00070D6C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007790C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008A754, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1600_EXFAT_H__
