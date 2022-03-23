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
#ifndef __FS_1400_H__
#define __FS_1400_H__

// Accessor vtable getters
#define FS_OFFSET_1400_SDMMC_ACCESSOR_GC   0x189F50
#define FS_OFFSET_1400_SDMMC_ACCESSOR_SD   0x18BD60
#define FS_OFFSET_1400_SDMMC_ACCESSOR_NAND 0x18A480

// Hooks
#define FS_OFFSET_1400_SDMMC_WRAPPER_READ  0x185AF0
#define FS_OFFSET_1400_SDMMC_WRAPPER_WRITE 0x185B50
#define FS_OFFSET_1400_RTLD                0x282B8
#define FS_OFFSET_1400_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1400_CLKRST_SET_MIN_V_CLK_RATE 0x1A5D90

// Misc funcs
#define FS_OFFSET_1400_LOCK_MUTEX          0x17E9F0
#define FS_OFFSET_1400_UNLOCK_MUTEX        0x17EA40

#define FS_OFFSET_1400_SDMMC_WRAPPER_CONTROLLER_OPEN  0x185AA0
#define FS_OFFSET_1400_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x185AD0

// Misc Data
#define FS_OFFSET_1400_SD_MUTEX            0xF2E3F0
#define FS_OFFSET_1400_NAND_MUTEX          0xF292F8
#define FS_OFFSET_1400_ACTIVE_PARTITION    0xF29338
#define FS_OFFSET_1400_SDMMC_DAS_HANDLE    0xDFE9C8

// NOPs
#define FS_OFFSET_1400_SD_DAS_INIT         0x27004

// Nintendo Paths
#define FS_OFFSET_1400_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006D9C0, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007AC24, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x000813E8, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0009387C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1400_H__
