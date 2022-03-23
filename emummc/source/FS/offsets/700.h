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
#ifndef __FS_700_H__
#define __FS_700_H__

// Accessor vtable getters
#define FS_OFFSET_700_SDMMC_ACCESSOR_GC   0x15BD90
#define FS_OFFSET_700_SDMMC_ACCESSOR_SD   0x15BB00
#define FS_OFFSET_700_SDMMC_ACCESSOR_NAND 0x157FF0

// Hooks
#define FS_OFFSET_700_SDMMC_WRAPPER_READ  0x14FDF0
#define FS_OFFSET_700_SDMMC_WRAPPER_WRITE 0x14FED0
#define FS_OFFSET_700_RTLD                0x5B4
#define FS_OFFSET_700_RTLD_DESTINATION    0x9C

#define FS_OFFSET_700_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_700_LOCK_MUTEX          0x148A90
#define FS_OFFSET_700_UNLOCK_MUTEX        0x148AE0

#define FS_OFFSET_700_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_700_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x14FD50

// Misc Data
#define FS_OFFSET_700_SD_MUTEX            0xF123E8
#define FS_OFFSET_700_NAND_MUTEX          0xF0DBE8
#define FS_OFFSET_700_ACTIVE_PARTITION    0xF0DC28
#define FS_OFFSET_700_SDMMC_DAS_HANDLE    0xE0E7A0

// NOPs
#define FS_OFFSET_700_SD_DAS_INIT         0x85FE8

// Nintendo Paths
#define FS_OFFSET_700_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x0007DA90, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007F344, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007F538, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007F914, .add_rel_offset = 4}, \
    {.opcode_reg = 4, .adrp_offset = 0x0007FAD8, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_700_H__
