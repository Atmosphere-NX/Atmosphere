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
#ifndef __FS_500_EXFAT_H__
#define __FS_500_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_500_EXFAT_SDMMC_ACCESSOR_GC   0xCF250
#define FS_OFFSET_500_EXFAT_SDMMC_ACCESSOR_SD   0xCEFD0
#define FS_OFFSET_500_EXFAT_SDMMC_ACCESSOR_NAND 0xCE990

// Hooks
#define FS_OFFSET_500_EXFAT_SDMMC_WRAPPER_READ  0xC9420
#define FS_OFFSET_500_EXFAT_SDMMC_WRAPPER_WRITE 0xC9500
#define FS_OFFSET_500_EXFAT_RTLD                0x584
#define FS_OFFSET_500_EXFAT_RTLD_DESTINATION    0x94

#define FS_OFFSET_500_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_500_EXFAT_LOCK_MUTEX          0x4080
#define FS_OFFSET_500_EXFAT_UNLOCK_MUTEX        0x40D0

#define FS_OFFSET_500_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_500_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0xC9380

// Misc Data
#define FS_OFFSET_500_EXFAT_SD_MUTEX            0xFA8268
#define FS_OFFSET_500_EXFAT_NAND_MUTEX          0xFA2E58
#define FS_OFFSET_500_EXFAT_ACTIVE_PARTITION    0xFA2E98
#define FS_OFFSET_500_EXFAT_SDMMC_DAS_HANDLE    0xFA1E30

// NOPs
#define FS_OFFSET_500_EXFAT_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_500_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x00028980, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0002ACE4, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0002B220, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_500_EXFAT_H__
