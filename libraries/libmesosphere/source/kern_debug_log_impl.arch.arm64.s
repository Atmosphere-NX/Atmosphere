/*
 * Copyright (c) Atmosphère-NX
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

/* ams::kern::KDebugLogImpl::PutStringBySemihosting(const char *str) */
.section    .text._ZN3ams4kern13KDebugLogImpl22PutStringBySemihostingEPKc, "ax", %progbits
.global     _ZN3ams4kern13KDebugLogImpl22PutStringBySemihostingEPKc
.type       _ZN3ams4kern13KDebugLogImpl22PutStringBySemihostingEPKc, %function
.balign 0x10
_ZN3ams4kern13KDebugLogImpl22PutStringBySemihostingEPKc:
    mov x1, x0
    mov x0, #0x4
    hlt #0xF000
    ret

