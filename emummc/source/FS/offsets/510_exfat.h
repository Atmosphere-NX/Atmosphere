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
#ifndef __FS_510_EXFAT_H__
#define __FS_510_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_510_EXFAT_SDMMC_ACCESSOR_GC   0xCF620
#define FS_OFFSET_510_EXFAT_SDMMC_ACCESSOR_SD   0xCF3A0
#define FS_OFFSET_510_EXFAT_SDMMC_ACCESSOR_NAND 0xCED60

// Hooks
#define FS_OFFSET_510_EXFAT_SDMMC_WRAPPER_READ  0xC97F0
#define FS_OFFSET_510_EXFAT_SDMMC_WRAPPER_WRITE 0xC98D0
#define FS_OFFSET_510_EXFAT_RTLD                0x584
#define FS_OFFSET_510_EXFAT_RTLD_DESTINATION    0x94

#define FS_OFFSET_510_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_510_EXFAT_LOCK_MUTEX          0x4080
#define FS_OFFSET_510_EXFAT_UNLOCK_MUTEX        0x40D0

#define FS_OFFSET_510_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_510_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0xC9750

// Misc Data
#define FS_OFFSET_510_EXFAT_SD_MUTEX            0xFA9268
#define FS_OFFSET_510_EXFAT_NAND_MUTEX          0xFA3E58
#define FS_OFFSET_510_EXFAT_ACTIVE_PARTITION    0xFA3E98
#define FS_OFFSET_510_EXFAT_SDMMC_DAS_HANDLE    0xFA2E30

// NOPs
#define FS_OFFSET_510_EXFAT_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_510_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x000289B0, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0002AD14, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0002B250, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_510_EXFAT_H__
