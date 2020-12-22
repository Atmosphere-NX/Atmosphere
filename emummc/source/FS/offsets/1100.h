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
#ifndef __FS_1100_H__
#define __FS_1100_H__

// Accessor vtable getters
#define FS_OFFSET_1100_SDMMC_ACCESSOR_GC   0x156D90
#define FS_OFFSET_1100_SDMMC_ACCESSOR_SD   0x154F40
#define FS_OFFSET_1100_SDMMC_ACCESSOR_NAND 0x1500F0

// Hooks
#define FS_OFFSET_1100_SDMMC_WRAPPER_READ  0x14B990
#define FS_OFFSET_1100_SDMMC_WRAPPER_WRITE 0x14BA70
#define FS_OFFSET_1100_RTLD                0x688
#define FS_OFFSET_1100_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1100_CLKRST_SET_MIN_V_CLK_RATE 0x14AC40

// Misc funcs
#define FS_OFFSET_1100_LOCK_MUTEX          0x28FF0
#define FS_OFFSET_1100_UNLOCK_MUTEX        0x29040

#define FS_OFFSET_1100_SDMMC_WRAPPER_CONTROLLER_OPEN  0x14B840
#define FS_OFFSET_1100_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x14B8F0

// Misc Data
#define FS_OFFSET_1100_SD_MUTEX            0xE323E8
#define FS_OFFSET_1100_NAND_MUTEX          0xE2D338
#define FS_OFFSET_1100_ACTIVE_PARTITION    0xE2D378
#define FS_OFFSET_1100_SDMMC_DAS_HANDLE    0xE15D40

// NOPs
#define FS_OFFSET_1100_SD_DAS_INIT         0x273B4

// Nintendo Paths
#define FS_OFFSET_1100_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006D944, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007A3C0, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x00080708, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00092198, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1100_H__
