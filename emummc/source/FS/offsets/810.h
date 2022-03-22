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
#ifndef __FS_810_H__
#define __FS_810_H__

// Accessor vtable getters
#define FS_OFFSET_810_SDMMC_ACCESSOR_GC   0x15EA20
#define FS_OFFSET_810_SDMMC_ACCESSOR_SD   0x15E790
#define FS_OFFSET_810_SDMMC_ACCESSOR_NAND 0x15AC80

// Hooks
#define FS_OFFSET_810_SDMMC_WRAPPER_READ  0x152A80
#define FS_OFFSET_810_SDMMC_WRAPPER_WRITE 0x152B60
#define FS_OFFSET_810_RTLD                0x5B4
#define FS_OFFSET_810_RTLD_DESTINATION    0x9C

#define FS_OFFSET_810_CLKRST_SET_MIN_V_CLK_RATE 0x16F370

// Misc funcs
#define FS_OFFSET_810_LOCK_MUTEX          0x14B6D0
#define FS_OFFSET_810_UNLOCK_MUTEX        0x14B720

#define FS_OFFSET_810_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_810_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x1529E0

// Misc Data
#define FS_OFFSET_810_SD_MUTEX            0xF1A3E8
#define FS_OFFSET_810_NAND_MUTEX          0xF15BE8
#define FS_OFFSET_810_ACTIVE_PARTITION    0xF15C28
#define FS_OFFSET_810_SDMMC_DAS_HANDLE    0xE167C0

// NOPs
#define FS_OFFSET_810_SD_DAS_INIT         0x87D58

// Nintendo Paths
#define FS_OFFSET_810_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x0007F5F0, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00081084, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00081278, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x00081654, .add_rel_offset = 4}, \
    {.opcode_reg = 4, .adrp_offset = 0x00081818, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0} \
}

#endif // __FS_810_H__
