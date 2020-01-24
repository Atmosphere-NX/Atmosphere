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
 
.macro GEN_USUAL_HANDLER name, index, lr_arm_displ, lr_thumb_displ
    _exception_handler_\name:
        ldr     sp, =_regs
        stmia   sp!, {r0-r7}

        /* Adjust lr to make it point to the location where the exception occured. */
        mrs     r1, spsr
        tst     r1, #0x20
        subeq   lr, lr, #\lr_arm_displ
        subne   lr, lr, #\lr_thumb_displ

        mov     r0, sp
        mov     r1, #\index
        b       _exception_handler_common
.endm

.section    .text.exception_handlers_asm, "ax", %progbits
.arm
.align      5

_exception_handler_common:
    mrs     r2, spsr
    mrs     r3, cpsr

    /* Mask interrupts. */
    orr     r3, #0xC0
    msr     cpsr_cx, r3

    /* Switch to the mode that triggered the interrupt, get the remaining regs, switch back. */
    ands    r4, r2, #0xF
    moveq   r4, #0xF             /* usr => sys */
    bic     r5, r3, #0xF
    orr     r5, r4
    msr     cpsr_c, r5
    stmia   r0!, {r8-lr}
    msr     cpsr_c, r3

    str     lr, [r0], #4
    str     r2, [r0]

    /* Finally, switch to system mode, setting interrupts and clearing the flags; set sp as well. */
    msr     cpsr_cxsf, #0xDF
    ldr     sp, =(_exception_handler_stack + 0x1000)
    ldr     r0, =_regs
    bl      exception_handler_main
    b       .

GEN_USUAL_HANDLER undefined_instruction, 1, 4, 2
GEN_USUAL_HANDLER swi, 2, 4, 2
GEN_USUAL_HANDLER prefetch_abort, 3, 4, 4
GEN_USUAL_HANDLER data_abort_normal, 4, 8, 8
GEN_USUAL_HANDLER fiq, 7, 4, 4

_exception_handler_data_abort:
    /* Mask interrupts (abort mode). */
    msr     cpsr_cx, #0xD7

    adr     sp, safecpy+8
    cmp     lr, sp
    blo     _exception_handler_data_abort_normal
    adr     sp, _safecpy_end+8
    cmp     lr, sp
    bhs     _exception_handler_data_abort_normal

    /* Set the flags, set r12 to 0 for safecpy, return from exception. */
    msr     spsr_f, #(1 << 30)
    mov     r12, #0
    subs    pc, lr, #4

.global safecpy
.type   safecpy, %function
safecpy:
    push    {r4, lr}
    mov     r3, #0
    movs    r12, #1

    _safecpy_loop:
        ldrb    r4, [r1, r3]
        cmp     r12, #0
        beq     _safecpy_loop_end
        strb    r4, [r0, r3]
        add     r3, #1
        cmp     r3, r2
        blo     _safecpy_loop

    _safecpy_loop_end:
    mov     r0, r3
    pop     {r4, lr}
    bx      lr         /* Need to do that separately on ARMv4. */

_safecpy_end:

.section    .rodata.exception_handlers_asm, "a", %progbits
.align      2
.global     exception_handler_table
exception_handler_table:
    .word   0   /* Reset */
    .word   _exception_handler_undefined_instruction
    .word   _exception_handler_swi
    .word   _exception_handler_prefetch_abort
    .word   _exception_handler_data_abort
    .word   0   /* Reserved */
    .word   0   /* IRQ */
    .word   _exception_handler_fiq

.section    .bss.exception_handlers_asm, "w", %nobits
.align      4
_exception_handler_stack: .skip 0x1000
_regs: .skip (4 * 17)
