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
#ifndef __FS_1500_EXFAT_H__
#define __FS_1500_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1500_EXFAT_SDMMC_ACCESSOR_GC   0x18EDB0
#define FS_OFFSET_1500_EXFAT_SDMMC_ACCESSOR_SD   0x190A30
#define FS_OFFSET_1500_EXFAT_SDMMC_ACCESSOR_NAND 0x18F270

// Hooks
#define FS_OFFSET_1500_EXFAT_SDMMC_WRAPPER_READ  0x18AC80
#define FS_OFFSET_1500_EXFAT_SDMMC_WRAPPER_WRITE 0x18ACE0
#define FS_OFFSET_1500_EXFAT_RTLD                0x26518
#define FS_OFFSET_1500_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1500_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1AB800

// Misc funcs
#define FS_OFFSET_1500_EXFAT_LOCK_MUTEX          0x184130
#define FS_OFFSET_1500_EXFAT_UNLOCK_MUTEX        0x184180

#define FS_OFFSET_1500_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x18AC40
#define FS_OFFSET_1500_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x18AC60

// Misc Data
#define FS_OFFSET_1500_EXFAT_SD_MUTEX            0x10053F0
#define FS_OFFSET_1500_EXFAT_NAND_MUTEX          0x10002E8
#define FS_OFFSET_1500_EXFAT_ACTIVE_PARTITION    0x1000328
#define FS_OFFSET_1500_EXFAT_SDMMC_DAS_HANDLE    0xFE08D8

// NOPs
#define FS_OFFSET_1500_EXFAT_SD_DAS_INIT         0x25454

// Nintendo Paths
#define FS_OFFSET_1500_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00063050, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0006FDE8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x000768D4, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00089364, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1500_EXFAT_H__
