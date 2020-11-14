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

/* ams::kern::svc::PatchSvcTableEntry(void (* const*)(), unsigned int, void (*)()) */
.section    .text._ZN3ams4kern3svc18PatchSvcTableEntryEPKPFvvEjS3_, "ax", %progbits
.global     _ZN3ams4kern3svc18PatchSvcTableEntryEPKPFvvEjS3_
.type       _ZN3ams4kern3svc18PatchSvcTableEntryEPKPFvvEjS3_, %function
_ZN3ams4kern3svc18PatchSvcTableEntryEPKPFvvEjS3_:
    /* This function violates const correctness by design, to patch the svc tables. */
    /* The svc tables live in .rodata (.rel.ro), but must be patched by initial constructors */
    /* to support firmware-specific table entries. */
    mov w1, w1
    str x2, [x0, x1, lsl #3]
    ret