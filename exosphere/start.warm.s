.align      6
.section    .text.warm.start, "ax", %progbits
.global     __start_warm
__start_warm:
        /* Nintendo copy-pasted https://github.com/ARM-software/arm-trusted-firmware/blob/master/plat/nvidia/tegra/common/aarch64/tegra_helpers.S#L312 */
        /*
        * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
        *
        * SPDX-License-Identifier: BSD-3-Clause
        */
    /* The following comments are mine. */
    /* mask all interrupts */
    msr daifset, daif

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
    b.eq _set_lock_and_sp
    mov  x0, xzr
    msr  oslar_el1, x0

    mov  x0, #(1 << 63)
    msr  cpuactlr_el1, x0 /* disable regional clock gating */
    isb
    mov  x0, #3
    msr  rmr_el3, x0
    isb
    dsb
    /* Nintendo forgot to copy-paste the branch instruction below. */
    _reset_wfi:
        wfi
        b _reset_wfi
.rept 65
    nop                     /* guard against speculative excecution */
.endr

    _set_lock_and_sp:
    /* set the OS lock */
    mov  x0, #1
    msr  oslar_el1, x0
    bl   __synchronize_cores

    /* set SP = SP_EL3 (handler stack) */
    msr  spsel, #1
    ldr  x20, =__warm_crt0_stack_top__
    mov  sp, x20
    bl   reconfigure_memory
    ldr  x16, =__init_warm
    br   x16

.section    .text.warm, "ax", %progbits
__init_warm:
    ldr  x20, =__warm_init_stack_top__
    mov  sp, x20
    b warmboot_main
