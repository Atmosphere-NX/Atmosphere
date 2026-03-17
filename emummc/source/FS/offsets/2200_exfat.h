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
#ifndef __FS_2200_EXFAT_H__
#define __FS_2200_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_2200_EXFAT_SDMMC_ACCESSOR_GC   0x1BB3B0
#define FS_OFFSET_2200_EXFAT_SDMMC_ACCESSOR_SD   0x1BD3B0
#define FS_OFFSET_2200_EXFAT_SDMMC_ACCESSOR_NAND 0x1BB9E0

// Hooks
#define FS_OFFSET_2200_EXFAT_SDMMC_WRAPPER_READ  0x1B72E0
#define FS_OFFSET_2200_EXFAT_SDMMC_WRAPPER_WRITE 0x1B7340
#define FS_OFFSET_2200_EXFAT_RTLD                0x2DED0
#define FS_OFFSET_2200_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2200_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1DA450

// Misc funcs
#define FS_OFFSET_2200_EXFAT_LOCK_MUTEX          0x1DA450
#define FS_OFFSET_2200_EXFAT_UNLOCK_MUTEX        0x1B0130

#define FS_OFFSET_2200_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1B72A0
#define FS_OFFSET_2200_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1B72C0

// Misc Data
#define FS_OFFSET_2200_EXFAT_SD_MUTEX            0x1002408
#define FS_OFFSET_2200_EXFAT_NAND_MUTEX          0xFFDD00
#define FS_OFFSET_2200_EXFAT_ACTIVE_PARTITION    0xFFDD40
#define FS_OFFSET_2200_EXFAT_SDMMC_DAS_HANDLE    0xFDBB98

// NOPs
#define FS_OFFSET_2200_EXFAT_SD_DAS_INIT         0x2B438

// Nintendo Paths
#define FS_OFFSET_2200_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x000715EC, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 3, .adrp_offset = 0x00082E64, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x0008B888, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x000A1B1C, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2200_EXFAT_H__