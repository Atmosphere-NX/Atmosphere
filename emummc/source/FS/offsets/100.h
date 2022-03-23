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
#ifndef __FS_100_H__
#define __FS_100_H__

// Accessor vtable getters
#define FS_OFFSET_100_SDMMC_ACCESSOR_GC   0x6F850
#define FS_OFFSET_100_SDMMC_ACCESSOR_SD   0x6F65C
#define FS_OFFSET_100_SDMMC_ACCESSOR_NAND 0x6F230

// Hooks
#define FS_OFFSET_100_SDMMC_WRAPPER_READ  0x6A930
#define FS_OFFSET_100_SDMMC_WRAPPER_WRITE 0x6A9F0
#define FS_OFFSET_100_RTLD                0x534
#define FS_OFFSET_100_RTLD_DESTINATION    0xA0

#define FS_OFFSET_100_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_100_LOCK_MUTEX          0x2884
#define FS_OFFSET_100_UNLOCK_MUTEX        0x28F0

#define FS_OFFSET_100_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_100_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x6A8AC

// Misc Data
#define FS_OFFSET_100_SD_MUTEX            0xE36058
#define FS_OFFSET_100_NAND_MUTEX          0xE30610
#define FS_OFFSET_100_ACTIVE_PARTITION    0xE30650
#define FS_OFFSET_100_SDMMC_DAS_HANDLE    0xE2F730

// NOPs
#define FS_OFFSET_100_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_100_NINTENDO_PATHS \
{ \
    {.opcode_reg = 8, .adrp_offset = 0x00032C58, .add_rel_offset = 8}, \
    {.opcode_reg = 9, .adrp_offset = 0x00032F40, .add_rel_offset = 8}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_100_H__
