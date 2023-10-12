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
#ifndef __FS_1700_EXFAT_H__
#define __FS_1700_EXFAT_H__

// Accessor vtable getters
#define FS_OFFSET_1700_EXFAT_SDMMC_ACCESSOR_GC   0x195B60
#define FS_OFFSET_1700_EXFAT_SDMMC_ACCESSOR_SD   0x197830
#define FS_OFFSET_1700_EXFAT_SDMMC_ACCESSOR_NAND 0x196030

// Hooks
#define FS_OFFSET_1700_EXFAT_SDMMC_WRAPPER_READ  0x191A20
#define FS_OFFSET_1700_EXFAT_SDMMC_WRAPPER_WRITE 0x191A80
#define FS_OFFSET_1700_EXFAT_RTLD                0x29D10
#define FS_OFFSET_1700_EXFAT_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1700_EXFAT_CLKRST_SET_MIN_V_CLK_RATE 0x1B29C0

// Misc funcs
#define FS_OFFSET_1700_EXFAT_LOCK_MUTEX          0x18AD00
#define FS_OFFSET_1700_EXFAT_UNLOCK_MUTEX        0x18AD50

#define FS_OFFSET_1700_EXFAT_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1919E0
#define FS_OFFSET_1700_EXFAT_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x191A00

// Misc Data
#define FS_OFFSET_1700_EXFAT_SD_MUTEX            0xFE03F0
#define FS_OFFSET_1700_EXFAT_NAND_MUTEX          0xFDBB78
#define FS_OFFSET_1700_EXFAT_ACTIVE_PARTITION    0xFDBBB8
#define FS_OFFSET_1700_EXFAT_SDMMC_DAS_HANDLE    0xFBC840

// NOPs
#define FS_OFFSET_1700_EXFAT_SD_DAS_INIT         0x28C64

// Nintendo Paths
#define FS_OFFSET_1700_EXFAT_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x00068068, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007510C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0007BEAC, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0008F674, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1700_EXFAT_H__
