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
#ifndef __FS_600_H__
#define __FS_600_H__

// Accessor vtable getters
#define FS_OFFSET_600_SDMMC_ACCESSOR_GC   0x153780
#define FS_OFFSET_600_SDMMC_ACCESSOR_SD   0x1534F0
#define FS_OFFSET_600_SDMMC_ACCESSOR_NAND 0x14F990

// Hooks
#define FS_OFFSET_600_SDMMC_WRAPPER_READ  0x1485A0
#define FS_OFFSET_600_SDMMC_WRAPPER_WRITE 0x148680
#define FS_OFFSET_600_RTLD                0x5B0
#define FS_OFFSET_600_RTLD_DESTINATION    0x98

#define FS_OFFSET_600_CLKRST_SET_MIN_V_CLK_RATE 0x0

// Misc funcs
#define FS_OFFSET_600_LOCK_MUTEX          0x1412C0
#define FS_OFFSET_600_UNLOCK_MUTEX        0x141310

#define FS_OFFSET_600_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_600_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x148500

// Misc Data
#define FS_OFFSET_600_SD_MUTEX            0xF06268
#define FS_OFFSET_600_NAND_MUTEX          0xF01BA0
#define FS_OFFSET_600_ACTIVE_PARTITION    0xF01BE0
#define FS_OFFSET_600_SDMMC_DAS_HANDLE    0xE01670

// NOPs
#define FS_OFFSET_600_SD_DAS_INIT         0x0

// Nintendo Paths
#define FS_OFFSET_600_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x000790DC, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007A924, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007AB18, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0007AEF4, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0} \
}

#endif // __FS_600_H__
