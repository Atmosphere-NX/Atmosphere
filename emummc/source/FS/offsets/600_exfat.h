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
#ifndef __FS_600_EXFAT_H__
#define __FS_600_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_600_EXFAT_SDMMC_ACCESSOR_GC   0x15EE80
#define FS_OFFSET_600_EXFAT_SDMMC_ACCESSOR_SD   0x15EBF0
#define FS_OFFSET_600_EXFAT_SDMMC_ACCESSOR_NAND 0x15B090

// Hooks
#define FS_OFFSET_600_EXFAT_SDMMC_WRAPPER_READ  0x153CA0
#define FS_OFFSET_600_EXFAT_SDMMC_WRAPPER_WRITE 0x153D80
#define FS_OFFSET_600_EXFAT_RTLD                0x5B0
#define FS_OFFSET_600_EXFAT_RTLD_DESTINATION    0x98

#define FS_OFFSET_600_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_600_EXFAT_LOCK_MUTEX          0x14C9C0
#define FS_OFFSET_600_EXFAT_UNLOCK_MUTEX        0x14CA10

#define FS_OFFSET_600_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_600_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x153C00

// Misc Data
#define FS_OFFSET_600_EXFAT_SD_MUTEX            0xFEB268
#define FS_OFFSET_600_EXFAT_NAND_MUTEX          0xFE6BA0
#define FS_OFFSET_600_EXFAT_ACTIVE_PARTITION    0xFE6BE0
#define FS_OFFSET_600_EXFAT_SDMMC_DAS_HANDLE    0xEE6670

// NOPs
#define FS_OFFSET_600_EXFAT_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_600_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x000847DC, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00086024, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00086218, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x000865F4, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0} \
}

#endif // __FS_600_EXFAT_H__
