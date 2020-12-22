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
#ifndef __FS_410_EXFAT_H__
#define __FS_410_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_410_EXFAT_SDMMC_ACCESSOR_GC   0xA33D8
#define FS_OFFSET_410_EXFAT_SDMMC_ACCESSOR_SD   0xA3178
#define FS_OFFSET_410_EXFAT_SDMMC_ACCESSOR_NAND 0xA2CE4

// Hooks
#define FS_OFFSET_410_EXFAT_SDMMC_WRAPPER_READ  0x9DC30
#define FS_OFFSET_410_EXFAT_SDMMC_WRAPPER_WRITE 0x9DCF0
#define FS_OFFSET_410_EXFAT_RTLD                0x4DC
#define FS_OFFSET_410_EXFAT_RTLD_DESTINATION    0x98

#define FS_OFFSET_410_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_410_EXFAT_LOCK_MUTEX          0x39A0
#define FS_OFFSET_410_EXFAT_UNLOCK_MUTEX        0x3A0C

#define FS_OFFSET_410_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_410_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x9DBAC

// Misc Data
#define FS_OFFSET_410_EXFAT_SD_MUTEX            0xF63268
#define FS_OFFSET_410_EXFAT_NAND_MUTEX          0xF5EC60
#define FS_OFFSET_410_EXFAT_ACTIVE_PARTITION    0xF5ECA0
#define FS_OFFSET_410_EXFAT_SDMMC_DAS_HANDLE    0xF5DBF0

// NOPs
#define FS_OFFSET_410_EXFAT_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_410_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x0002023C, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00021BE8, .add_rel_offset = 12}, \
    {.opcode_reg = 3, .adrp_offset = 0x00021EC4, .add_rel_offset = 12}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_410_EXFAT_H__
