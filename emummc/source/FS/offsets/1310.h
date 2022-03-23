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
#ifndef __FS_1310_H__
#define __FS_1310_H__

// Accessor vtable getters
#define FS_OFFSET_1310_SDMMC_ACCESSOR_GC   0x158C20
#define FS_OFFSET_1310_SDMMC_ACCESSOR_SD   0x15AA30
#define FS_OFFSET_1310_SDMMC_ACCESSOR_NAND 0x159150

// Hooks
#define FS_OFFSET_1310_SDMMC_WRAPPER_READ  0x1545C0
#define FS_OFFSET_1310_SDMMC_WRAPPER_WRITE 0x154680
#define FS_OFFSET_1310_RTLD                0x688
#define FS_OFFSET_1310_RTLD_DESTINATION    ((uintptr_t)(INT64_C(-0x3C)))

#define FS_OFFSET_1310_CLKRST_SET_MIN_V_CLK_RATE 0x1537C0

// Misc funcs
#define FS_OFFSET_1310_LOCK_MUTEX          0x29690
#define FS_OFFSET_1310_UNLOCK_MUTEX        0x296E0

#define FS_OFFSET_1310_SDMMC_WRAPPER_CONTROLLER_OPEN  0x1544A0
#define FS_OFFSET_1310_SDMMC_WRAPPER_CONTROLLER_CLOSE 0x154530

// Misc Data
#define FS_OFFSET_1310_SD_MUTEX            0xE133E8
#define FS_OFFSET_1310_NAND_MUTEX          0xE0E768
#define FS_OFFSET_1310_ACTIVE_PARTITION    0xE0E7A8
#define FS_OFFSET_1310_SDMMC_DAS_HANDLE    0xDF6E18

// NOPs
#define FS_OFFSET_1310_SD_DAS_INIT         0x27744

// Nintendo Paths
#define FS_OFFSET_1310_NINTENDO_PATHS \
{ \
	{.opcode_reg = 3, .adrp_offset = 0x0006EBE0, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 3, .adrp_offset = 0x0007BEEC, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x00082294, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 4, .adrp_offset = 0x0009422C, .add_rel_offset = 0x00000004}, \
	{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \
}

#endif // __FS_1310_H__
