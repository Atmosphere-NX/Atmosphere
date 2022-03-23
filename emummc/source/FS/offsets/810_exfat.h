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
#ifndef __FS_810_EXFAT_H__
#define __FS_810_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_810_EXFAT_SDMMC_ACCESSOR_GC   0x169FD0
#define FS_OFFSET_810_EXFAT_SDMMC_ACCESSOR_SD   0x169D40
#define FS_OFFSET_810_EXFAT_SDMMC_ACCESSOR_NAND 0x166230

// Hooks
#define FS_OFFSET_810_EXFAT_SDMMC_WRAPPER_READ  0x15E030
#define FS_OFFSET_810_EXFAT_SDMMC_WRAPPER_WRITE 0x15E110
#define FS_OFFSET_810_EXFAT_RTLD                0x5B4
#define FS_OFFSET_810_EXFAT_RTLD_DESTINATION    0x9C

#define FS_OFFSET_810_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x17A920

// Misc funcs
#define FS_OFFSET_810_EXFAT_LOCK_MUTEX          0x156C80
#define FS_OFFSET_810_EXFAT_UNLOCK_MUTEX        0x156CD0

#define FS_OFFSET_810_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0
#define FS_OFFSET_810_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x15DF90

// Misc Data
#define FS_OFFSET_810_EXFAT_SD_MUTEX            0xFFE3E8
#define FS_OFFSET_810_EXFAT_NAND_MUTEX          0xFF9BE8
#define FS_OFFSET_810_EXFAT_ACTIVE_PARTITION    0xFF9C28
#define FS_OFFSET_810_EXFAT_SDMMC_DAS_HANDLE    0xEFAA20

// NOPs
#define FS_OFFSET_810_EXFAT_SD_DAS_INIT         0x93308

// Nintendo Paths
#define FS_OFFSET_810_EXFAT_NINTENDO_PATHS \
{ \
    {.opcode_reg = 3, .adrp_offset = 0x0008ABA0, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008C634, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008C828, .add_rel_offset = 4}, \
    {.opcode_reg = 3, .adrp_offset = 0x0008CC04, .add_rel_offset = 4}, \
    {.opcode_reg = 4, .adrp_offset = 0x0008CDC8, .add_rel_offset = 4}, \
    {.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0} \
}

#endif // __FS_810_EXFAT_H__
