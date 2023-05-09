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
#ifndef __FS_1603_EXFAT_H__
#define __FS_1603_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1603_EXFAT_SDMMC_ACCESSOR_GC   0x190FD0
#define FS_OFFSET_1603_EXFAT_SDMMC_ACCESSOR_SD   0x192C50
#define FS_OFFSET_1603_EXFAT_SDMMC_ACCESSOR_NAND 0x191490

// Hooks
#define FS_OFFSET_1603_EXFAT_SDMMC_WRAPPER_READ  0x18CF20
#define FS_OFFSET_1603_EXFAT_SDMMC_WRAPPER_WRITE 0x18CF80
#define FS_OFFSET_1603_EXFAT_RTLD                0x269B0
#define FS_OFFSET_1603_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1603_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1ADA60

// Misc funcs
#define FS_OFFSET_1603_EXFAT_LOCK_MUTEX          0x186460
#define FS_OFFSET_1603_EXFAT_UNLOCK_MUTEX        0x1864B0

#define FS_OFFSET_1603_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x18CEE0
#define FS_OFFSET_1603_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x18CF00

// Misc Data
#define FS_OFFSET_1603_EXFAT_SD_MUTEX            0x100D3F0
#define FS_OFFSET_1603_EXFAT_NAND_MUTEX          0x1008B58
#define FS_OFFSET_1603_EXFAT_ACTIVE_PARTITION    0x1008B98
#define FS_OFFSET_1603_EXFAT_SDMMC_DAS_HANDLE    0xFE98B0

// NOPs
#define FS_OFFSET_1603_EXFAT_SD_DAS_INIT         0x258D4

// Nintendo Paths
#define FS_OFFSET_1603_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00063B98, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00070DBC, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007795C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008A7A4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1603_EXFAT_H__
