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

/* ams::kern::svc::CallCallSecureMonitor64From32() */
.section    .text._ZN3ams4kern3svc29CallCallSecureMonitor64From32Ev, "ax", %progbits
.global     _ZN3ams4kern3svc29CallCallSecureMonitor64From32Ev
.type       _ZN3ams4kern3svc29CallCallSecureMonitor64From32Ev, %function
_ZN3ams4kern3svc29CallCallSecureMonitor64From32Ev:
    /* Secure Monitor 64-from-32 ABI is not supported. */
    mov     x0, xzr
    mov     x1, xzr
    mov     x2, xzr
    mov     x3, xzr
    mov     x4, xzr
    mov     x5, xzr
    mov     x6, xzr
    mov     x7, xzr

    ret