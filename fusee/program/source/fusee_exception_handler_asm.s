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

.section .text._ZN3ams6nxboot17ExceptionHandlerNEl, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandlerNEl
.type   _ZN3ams6nxboot17ExceptionHandlerNEl, %function
_ZN3ams6nxboot17ExceptionHandlerNEl:
    /* Get the normal return address. */
    mov r1, lr

    /* Get the SVC return address. */
    msr cpsr_cf, #0xD3
    mov r2, lr

    /* Invoke the exception handler in abort mode. */
    msr cpsr_cf, #0xD7
    b _ZN3ams6nxboot20ExceptionHandlerImplElmm


.section .text._ZN3ams6nxboot16ExceptionHandlerEv, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot16ExceptionHandlerEv
.type   _ZN3ams6nxboot16ExceptionHandlerEv, %function
_ZN3ams6nxboot16ExceptionHandlerEv:
    mov r0, #-1
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler0Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler0Ev
.type   _ZN3ams6nxboot17ExceptionHandler0Ev, %function
_ZN3ams6nxboot17ExceptionHandler0Ev:
    mov r0, #0
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler1Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler1Ev
.type   _ZN3ams6nxboot17ExceptionHandler1Ev, %function
_ZN3ams6nxboot17ExceptionHandler1Ev:
    mov r0, #1
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler2Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler2Ev
.type   _ZN3ams6nxboot17ExceptionHandler2Ev, %function
_ZN3ams6nxboot17ExceptionHandler2Ev:
    mov r0, #2
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler3Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler3Ev
.type   _ZN3ams6nxboot17ExceptionHandler3Ev, %function
_ZN3ams6nxboot17ExceptionHandler3Ev:
    mov r0, #3
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler4Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler4Ev
.type   _ZN3ams6nxboot17ExceptionHandler4Ev, %function
_ZN3ams6nxboot17ExceptionHandler4Ev:
    mov r0, #4
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler5Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler5Ev
.type   _ZN3ams6nxboot17ExceptionHandler5Ev, %function
_ZN3ams6nxboot17ExceptionHandler5Ev:
    mov r0, #5
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler6Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler6Ev
.type   _ZN3ams6nxboot17ExceptionHandler6Ev, %function
_ZN3ams6nxboot17ExceptionHandler6Ev:
    mov r0, #6
    b _ZN3ams6nxboot17ExceptionHandlerNEl

.section .text._ZN3ams6nxboot17ExceptionHandler7Ev, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot17ExceptionHandler7Ev
.type   _ZN3ams6nxboot17ExceptionHandler7Ev, %function
_ZN3ams6nxboot17ExceptionHandler7Ev:
    mov r0, #7
    b _ZN3ams6nxboot17ExceptionHandlerNEl
