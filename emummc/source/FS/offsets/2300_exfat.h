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
#ifndef __FS_2300_EXFAT_H__
#define __FS_2300_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_2300_EXFAT_SDMMC_ACCESSOR_GC   0x1BE840
#define FS_OFFSET_2300_EXFAT_SDMMC_ACCESSOR_SD   0x1C0840
#define FS_OFFSET_2300_EXFAT_SDMMC_ACCESSOR_NAND 0x1BEE70

// Hooks
#define FS_OFFSET_2300_EXFAT_SDMMC_WRAPPER_READ  0x1BA770
#define FS_OFFSET_2300_EXFAT_SDMMC_WRAPPER_WRITE 0x1BA7D0
#define FS_OFFSET_2300_EXFAT_RTLD                0x2F530
#define FS_OFFSET_2300_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2300_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1DD9E0

// Misc funcs
#define FS_OFFSET_2300_EXFAT_LOCK_MUTEX          0x1B3290
#define FS_OFFSET_2300_EXFAT_UNLOCK_MUTEX        0x1B32E0

#define FS_OFFSET_2300_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1BA730
#define FS_OFFSET_2300_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1BA750

// Misc Data
#define FS_OFFSET_2300_EXFAT_SD_MUTEX            0x1006408
#define FS_OFFSET_2300_EXFAT_NAND_MUTEX          0x1001D70
#define FS_OFFSET_2300_EXFAT_ACTIVE_PARTITION    0x1001DB0
#define FS_OFFSET_2300_EXFAT_SDMMC_DAS_HANDLE    0xFDFBE8

// NOPs
#define FS_OFFSET_2300_EXFAT_SD_DAS_INIT         0x2CAA8

// Nintendo Paths
#define FS_OFFSET_2300_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x00072D8C, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 3, .adrp_offset = 0x00081BB4, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x0008A678, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x0009F98C, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2300_EXFAT_H__