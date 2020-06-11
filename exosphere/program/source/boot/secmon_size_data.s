/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

.section    .metadata.sizes, "ax", %progbits
.align      6
.global     __metadata__sizes
__metadata__sizes:
    .quad 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB
    .quad __glob_start__
    .quad __bootcode_start__
    .quad __bootcode_end__
    .quad __program_start__
    .quad 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD
