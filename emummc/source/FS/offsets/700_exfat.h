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
#ifndef __FS_700_EXFAT_H__
#define __FS_700_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_700_EXFAT_SDMMC_ACCESSOR_GC   0x167340
#define FS_OFFSET_700_EXFAT_SDMMC_ACCESSOR_SD   0x1670B0
#define FS_OFFSET_700_EXFAT_SDMMC_ACCESSOR_NAND 0x1635A0

// Hooks
#define FS_OFFSET_700_EXFAT_SDMMC_WRAPPER_READ  0x15B3A0
#define FS_OFFSET_700_EXFAT_SDMMC_WRAPPER_WRITE 0x15B480
#define FS_OFFSET_700_EXFAT_RTLD                0x5B4
#define FS_OFFSET_700_EXFAT_RTLD_DESTINATION    0x9C

#define FS_OFFSET_700_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_700_EXFAT_LOCK_MUTEX          0x154040
#define FS_OFFSET_700_EXFAT_UNLOCK_MUTEX        0x154090

#define FS_OFFSET_700_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_700_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x15B300

// Misc Data
#define FS_OFFSET_700_EXFAT_SD_MUTEX            0xFF73E8
#define FS_OFFSET_700_EXFAT_NAND_MUTEX          0xFF2BE8
#define FS_OFFSET_700_EXFAT_ACTIVE_PARTITION    0xFF2C28
#define FS_OFFSET_700_EXFAT_SDMMC_DAS_HANDLE    0xEF3A00

// NOPs
#define FS_OFFSET_700_EXFAT_SD_DAS_INIT         0x91598

// Nintendo Paths
#define FS_OFFSET_700_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x00089040, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008A8F4, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008AAE8, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008AEC4, .add_rel_offset = 4}, \
    {.opcode_reg = 4, .adrp_offset = 0x0008B088, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_700_EXFAT_H__
