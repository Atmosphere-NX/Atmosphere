/* For some reason GAS doesn't know about it, even with .cpu cortex-a57 */
#define cpuactlr_el1 s3_1_c15_c2_0
.macro      ERRATUM_INVALIDATE_BTB_AT_BOOT
/* Nintendo copy-pasted https://github.com/ARM-software/arm-trusted-firmware/blob/master/plat/nvidia/tegra/common/aarch64/tegra_helpers.S#L312 */
        /*
        * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
        *
        * SPDX-License-Identifier: BSD-3-Clause
        */
    /* The following comments are mine. */
    /* mask all interrupts */
    msr daifset, 0b1111

    /*
        Enable invalidates of branch target buffer, then flush
        the entire instruction cache at the local level, and
        with the reg change, the branch target buffer, then disable
        invalidates of the branch target buffer again.
    */
    mrs  x0, cpuactlr_el1
    orr  x0, x0, #1
    msr  cpuactlr_el1, x0

    dsb  sy
    isb
    ic   iallu
    dsb  sy
    isb

    mrs  x0, cpuactlr_el1
    bic  x0, x0, #1
    msr  cpuactlr_el1, x0

.rept 7
    nop                     /* wait long enough for the write to cpuactlr_el1 to have completed */
.endr

    /* if the OS lock is set, disable it and request a warm reset */
    mrs  x0, oslsr_el1
    ands x0, x0, #2
    b.eq 2f
    mov  x0, xzr
    msr  oslar_el1, x0

    mov  x0, #(1 << 63)
    msr  cpuactlr_el1, x0 /* disable regional clock gating */
    isb
    mov  x0, #3
    msr  rmr_el3, x0
    isb
    dsb  sy
    /* Nintendo forgot to copy-paste the branch instruction below. */
    1:
        wfi
        b 1b
.rept 65
    nop                     /* guard against speculative excecution */
.endr

    2:
    /* set the OS lock */
    mov  x0, #1
    msr  oslar_el1, x0
.endm

.align      6
.section    .text.cold.start, "ax", %progbits
.global     __start_cold
__start_cold:
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    msr  spsel, #0
    bl   get_coldboot_crt0_stack_address /* should be optimized so it doesn't make function calls */
    mov  sp, x0
    bl   coldboot_init
    ldr  x16, =__jump_to_main_cold
    br   x16

.align      6
.section    .text.warm.start, "ax", %progbits
.global     __start_warm
__start_warm:
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    /* For some reasons, Nintendo uses spsel, #1 here, causing issues if an exception occurs */
    msr  spsel, #0
    bl   get_warmboot_crt0_stack_address /* should be optimized so it doesn't make function calls */
    mov  sp, x0
    bl   warmboot_init
    ldr  x16, =__jump_to_main_warm
    br   x16

.section    .text.__jump_to_main_cold, "ax", %progbits
__jump_to_main_cold:
    bl   __set_exception_entry_stack_pointer

    bl   get_pk2ldr_stack_address 
    mov  sp, x0
    bl   load_package2

    mov  w0, #3 /* use core3 stack temporarily */
    bl   get_exception_entry_stack_address
    mov  sp, x0
    b    coldboot_main

.section    .text.__jump_to_main_warm, "ax", %progbits
__jump_to_main_warm:
    /* Nintendo doesn't do that here, causing issues if an exception occurs */
    bl   __set_exception_entry_stack_pointer

    bl   get_pk2ldr_stack_address 
    mov  sp, x0
    bl   load_package2

    mov  w0, #3 /* use core0,1,2 stack bottom + 0x800 (VA of warmboot crt0 sp) temporarily */
    bl   get_exception_entry_stack_address
    add  sp, x0, #0x800
    b    warmboot_main

.section    .text.__set_exception_entry_stack, "ax", %progbits
.type       __set_exception_entry_stack, %function
.global     __set_exception_entry_stack
__set_exception_entry_stack_pointer:
    /* If SPSel == 1 on entry, make sure your function doesn't use stack variables! */
    mov  x16, lr
    mrs  x17, spsel
    mrs  x0, mpidr_el1
    and  w0, w0, #3
    bl   get_exception_entry_stack_address /* should be optimized so it doesn't make function calls */
    msr  spsel, #1
    mov  sp, x0
    msr  spsel, x17
    mov  lr, x16
    ret

.section    .text.__jump_to_lower_el, "ax", %progbits
.global     __jump_to_lower_el
.type       __jump_to_lower_el, %function
__jump_to_lower_el:
    /* x0: arg (context ID), x1: entrypoint, w2: exception level */
    msr  elr_el3, x1

    mov  w1, #((0b1111 << 6) | 1) /* DAIF set and SP = SP_ELx*/
    orr  w1, w2, w2, lsl#2
    msr  spsr_el3, x1

    bl __set_exception_entry_stack_pointer

    isb
    eret
