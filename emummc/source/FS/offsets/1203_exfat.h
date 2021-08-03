/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
 * Copyright (c) 2021 CTCaer
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
#ifndef __FS_1203_EXFAT_H__
#define __FS_1203_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1203_EXFAT_SDMMC_ACCESSOR_GC   0x1550E0
#define FS_OFFSET_1203_EXFAT_SDMMC_ACCESSOR_SD   0x156EF0
#define FS_OFFSET_1203_EXFAT_SDMMC_ACCESSOR_NAND 0x155610

// Hooks
#define FS_OFFSET_1203_EXFAT_SDMMC_WRAPPER_READ  0x150A80
#define FS_OFFSET_1203_EXFAT_SDMMC_WRAPPER_WRITE 0x150B40
#define FS_OFFSET_1203_EXFAT_RTLD                0x688
#define FS_OFFSET_1203_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1203_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x14FDD0

// Misc funcs
#define FS_OFFSET_1203_EXFAT_LOCK_MUTEX          0x29350
#define FS_OFFSET_1203_EXFAT_UNLOCK_MUTEX        0x293A0

#define FS_OFFSET_1203_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x150960
#define FS_OFFSET_1203_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1509F0

// Misc Data
#define FS_OFFSET_1203_EXFAT_SD_MUTEX            0xE4B3E8
#define FS_OFFSET_1203_EXFAT_NAND_MUTEX          0xE46768
#define FS_OFFSET_1203_EXFAT_ACTIVE_PARTITION    0xE467A8
#define FS_OFFSET_1203_EXFAT_SDMMC_DAS_HANDLE    0xE2EDB0

// NOPs
#define FS_OFFSET_1203_EXFAT_SD_DAS_INIT         0x27244

// Nintendo Paths
#define FS_OFFSET_1203_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006E920, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007AFD0, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00081364, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00092960, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1203_EXFAT_H__
