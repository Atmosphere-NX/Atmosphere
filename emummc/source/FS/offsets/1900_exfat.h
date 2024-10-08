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
#ifndef __FS_1900_EXFAT_H__
#define __FS_1900_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1900_EXFAT_SDMMC_ACCESSOR_GC   0x1A1430
#define FS_OFFSET_1900_EXFAT_SDMMC_ACCESSOR_SD   0x1A37B0
#define FS_OFFSET_1900_EXFAT_SDMMC_ACCESSOR_NAND 0x1A1BE0

// Hooks
#define FS_OFFSET_1900_EXFAT_SDMMC_WRAPPER_READ  0x19D2A0
#define FS_OFFSET_1900_EXFAT_SDMMC_WRAPPER_WRITE 0x19D300
#define FS_OFFSET_1900_EXFAT_RTLD                0x275F0
#define FS_OFFSET_1900_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x50)))

#define FS_OFFSET_1900_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1BF0B0

// Misc funcs
#define FS_OFFSET_1900_EXFAT_LOCK_MUTEX          0x196450
#define FS_OFFSET_1900_EXFAT_UNLOCK_MUTEX        0x1964A0

#define FS_OFFSET_1900_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x19D260
#define FS_OFFSET_1900_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x19D280

// Misc Data
#define FS_OFFSET_1900_EXFAT_SD_MUTEX            0xFF4408
#define FS_OFFSET_1900_EXFAT_NAND_MUTEX          0xFEFB60
#define FS_OFFSET_1900_EXFAT_ACTIVE_PARTITION    0xFEFBA0
#define FS_OFFSET_1900_EXFAT_SDMMC_DAS_HANDLE    0xFCF908

// NOPs
#define FS_OFFSET_1900_EXFAT_SD_DAS_INIT         0x260C4

// Nintendo Paths
#define FS_OFFSET_1900_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00067FC8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00075D6C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007D1E8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00092818, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1900_EXFAT_H__
