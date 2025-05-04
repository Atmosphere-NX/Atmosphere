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

/* For some reason GAS doesn't know about it, even with .cpu cortex-a57 */
#define cpuactlr_el1 s3_1_c15_c2_0
#define cpuectlr_el1 s3_1_c15_c2_1

.macro      RESET_CORE
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
.endm

.macro      ERRATUM_INVALIDATE_BTB_AT_BOOT
/* Nintendo copy-pasted https://github.com/ARM-software/arm-trusted-firmware/blob/master/plat/nvidia/tegra/common/aarch64/tegra_helpers.S#L312 */
        /*
        * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
        *
        * SPDX-License-Identifier: BSD-3-Clause
        */
    /* The following comments are mine. */

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

    RESET_CORE

.rept 65
    nop                     /* guard against speculative excecution */
.endr

    2:
    /* set the OS lock */
    mov  x0, #1
    msr  oslar_el1, x0
.endm

.section    .crt0.text.start, "ax", %progbits
.align      6
.global     _start
_start:
    /* mask all interrupts */
    msr daifset, #0xF

    /* Fixup hardware erratum */
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    /* Set the stack pointer to a temporary location. */
    ldr x20, =0x7C020000
    mov sp, x20

    /* Uncompress the program and iram boot code images. */
    b _ZN3ams6secmon6loader20UncompressAndExecuteEv
