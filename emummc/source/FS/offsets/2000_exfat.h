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
#ifndef __FS_2000_EXFAT_H__
#define __FS_2000_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_2000_EXFAT_SDMMC_ACCESSOR_GC   0x1B36D0
#define FS_OFFSET_2000_EXFAT_SDMMC_ACCESSOR_SD   0x1B5A50
#define FS_OFFSET_2000_EXFAT_SDMMC_ACCESSOR_NAND 0x1B3E80

// Hooks
#define FS_OFFSET_2000_EXFAT_SDMMC_WRAPPER_READ  0x1AF540
#define FS_OFFSET_2000_EXFAT_SDMMC_WRAPPER_WRITE 0x1AF5A0
#define FS_OFFSET_2000_EXFAT_RTLD                0x2B594
#define FS_OFFSET_2000_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2000_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1D1A70

// Misc funcs
#define FS_OFFSET_2000_EXFAT_LOCK_MUTEX          0x1A86A0
#define FS_OFFSET_2000_EXFAT_UNLOCK_MUTEX        0x1A86F0

#define FS_OFFSET_2000_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1AF500
#define FS_OFFSET_2000_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1AF520

// Misc Data
#define FS_OFFSET_2000_EXFAT_SD_MUTEX            0x1006408
#define FS_OFFSET_2000_EXFAT_NAND_MUTEX          0x1001CF0
#define FS_OFFSET_2000_EXFAT_ACTIVE_PARTITION    0x1001D30
#define FS_OFFSET_2000_EXFAT_SDMMC_DAS_HANDLE    0xFDFB08

// NOPs
#define FS_OFFSET_2000_EXFAT_SD_DAS_INIT         0x289F4

// Nintendo Paths
#define FS_OFFSET_2000_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006DB14, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007CE1C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00084A08, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0009AE48, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2000_EXFAT_H__
