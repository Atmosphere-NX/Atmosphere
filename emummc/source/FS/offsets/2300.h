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
#ifndef __FS_2300_H__
#define __FS_2300_H__

// Accessor vtable getters
#define FS_OFFSET_2300_SDMMC_ACCESSOR_GC   0x1B3640
#define FS_OFFSET_2300_SDMMC_ACCESSOR_SD   0x1B5640
#define FS_OFFSET_2300_SDMMC_ACCESSOR_NAND 0x1B3C70

// Hooks
#define FS_OFFSET_2300_SDMMC_WRAPPER_READ  0x1AF570
#define FS_OFFSET_2300_SDMMC_WRAPPER_WRITE 0x1AF5D0
#define FS_OFFSET_2300_RTLD                0x2F520
#define FS_OFFSET_2300_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x4C)))

#define FS_OFFSET_2300_CLKRST_SET_MIN_V_CLK_RATE 0x1D27E0

// Misc funcs
#define FS_OFFSET_2300_LOCK_MUTEX          0x1A8090
#define FS_OFFSET_2300_UNLOCK_MUTEX        0x1A80E0

#define FS_OFFSET_2300_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1AF530
#define FS_OFFSET_2300_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1AF550

// Misc Data
#define FS_OFFSET_2300_SD_MUTEX            0xFF5408
#define FS_OFFSET_2300_NAND_MUTEX          0xFF0D70
#define FS_OFFSET_2300_ACTIVE_PARTITION    0xFF0DB0
#define FS_OFFSET_2300_SDMMC_DAS_HANDLE    0xFD2BE8

// NOPs
#define FS_OFFSET_2300_SD_DAS_INIT         0x2CA98

// Nintendo Paths
#define FS_OFFSET_2300_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x00072D7C, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 3, .adrp_offset = 0x00081BA4, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x0008A668, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 4, .adrp_offset = 0x0009F97C, .add_rel_offset = 0x00000004}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_2300_H__
