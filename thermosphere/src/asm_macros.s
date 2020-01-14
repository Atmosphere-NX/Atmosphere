/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#define EXCEP_STACK_FRAME_SIZE      0x140

#define CORECTX_USER_FRAME_OFFSET   0x000
#define CORECTX_SCRATCH_OFFSET      0x008
#define CORECTX_CRASH_STACK_OFFSET  0x010

.macro FUNCTION name
    .section        .text.\name, "ax", %progbits
    .global         \name
    .type           \name, %function
    .func           \name
    .cfi_sections   .debug_frame
    .cfi_startproc
    \name:
.endm

.macro END_FUNCTION
    .cfi_endproc
    .endfunc
.endm
