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

.section    .warmboot.text.start, "ax", %progbits
.align      4
.global     _start_warm
_start_warm:
    /* mask all interrupts */
    msr daifset, #0xF

    /* Fixup hardware erratum */
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    /* Acquire exclusive access to the common warmboot stack. */
    bl _ZN3ams6secmon26AcquireCommonWarmbootStackEv

    /* Set the stack pointer to the common warmboot stack address. */
    msr  spsel, #1
    ldr x20, =0x7C0107C0
    mov sp, x20

    /* Perform warmboot setup. */
    bl _ZN3ams6secmon59SetupSocDmaControllersCpuMemoryControllersEnableMmuWarmbootEv

    /* Jump to the newly-mapped virtual address. */
    b _ZN3ams6secmon20StartWarmbootVirtualEv


/* void ams::secmon::AcquireCommonWarmbootStack() { */
/* NOTE: This implements critical section enter via https://en.wikipedia.org/wiki/Lamport%27s_bakery_algorithm */
/* This algorithm is used because the MMU is not awake yet, so exclusive load/store instructions are not usable. */
/* NOTE: Nintendo attempted to implement this algorithm themselves, but did not really understand how it works. */
/* They use the same ticket number for all cores; this can lead to starvation and other problems. */
.section    .warmboot.text._ZN3ams6secmon26AcquireCommonWarmbootStackEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon26AcquireCommonWarmbootStackEv
_ZN3ams6secmon26AcquireCommonWarmbootStackEv:
    /* BakeryLock *lock = std::addressof(secmon::CommonWarmBootStackLock); */
    ldr x0, =_ZN3ams6secmon23CommonWarmbootStackLockE

    /* const u32 id = GetCurrentCoreId(); */
    mrs x8, mpidr_el1
    and x8, x8, #3

    /* lock->customers[id].is_entering = true; */
    ldrb w2, [x0, x8]
    orr  w2, w2, #~0x7F
    strb w2, [x0, x8]

    /* const u8 ticket_0 = lock->customers[0].ticket_number; */
    ldrb w4, [x0, #0]
    and  w4, w4, #0x7F

    /* const u8 ticket_1 = lock->customers[1].ticket_number; */
    ldrb w5, [x0, #1]
    and  w5, w5, #0x7F

    /* const u8 ticket_2 = lock->customers[2].ticket_number; */
    ldrb w6, [x0, #2]
    and  w6, w6, #0x7F

    /* const u8 ticket_3 = lock->customers[3].ticket_number; */
    ldrb w7, [x0, #3]
    and  w7, w7, #0x7F

    /* u8 biggest_ticket = std::max(std::max(ticket_0, ticket_1), std::max(ticket_2, ticket_3)) */
    cmp  w4, w5
    csel w2, w4, w5, hi
    cmp  w6, w7
    csel w3, w6, w7, hi
    cmp  w2, w3
    csel w2, w2, w3, hi

    /* NOTE: The biggest a ticket can ever be is 4, so the general increment is safe and 7-bit increment is not needed. */
    /* lock->customers[id] = { .is_entering = false, .ticket_number = ++biggest_ticket }; */
    add  w2, w2, #1
    strb w2, [x0, x8]

    /* Ensure instructions aren't reordered around this point. */
    /* hw::DataSynchronizationBarrier(); */
    dsb  sy

    /* hw::SendEvent(); */
    sev

    /* for (unsigned int i = 0; i < 4; ++i) { */
    mov  w3, #0
1:
    /*     hw::SendEventLocal(); */
    sevl

    /*     do { */
2:
    /*         hw::WaitForEvent(); */
    wfe
    /*     while (lock->customers[i].is_entering); */
    ldrb w4, [x0, x3]
    tbnz w4, #7, 2b

    /*     u8 their_ticket; */

    /*     hw::SendEventLocal(); */
    sevl

    /*    do { */
2:
    /*         hw::WaitForEvent(); */
    wfe
    /*         their_ticket = lock->customers[i].ticket_number; */
    ldrb w4, [x0, x3]
    ands w4, w4, #0x7F
    /*         if (their_ticket == 0) { break; } */
    b.eq 3f
    /*    while ((their_ticket > my_ticket) || (their_ticket == my_ticket && id > i)); */
    cmp  w2, w4
    b.hi 2b
    ccmp w8, w3, #0, eq
    b.hi 2b

    /* } */
3:
    add w3, w3, #1
    cmp w3, #4
    b.ne 1b

    /* hw::DataMemoryBarrier(); */
    dmb sy

    ret
