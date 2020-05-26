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

.section    .text._ZN3ams6secmon3smc19PivotStackAndInvokeEPvPFvvE, "ax", %progbits
.align      4
.global     _ZN3ams6secmon3smc19PivotStackAndInvokeEPvPFvvE
_ZN3ams6secmon3smc19PivotStackAndInvokeEPvPFvvE:
    /* Pivot to use the provided stack pointer. */
    mov  sp, x0

    /* Release our lock on the common smc stack. */
    mov  x19, x1
    bl   _ZN3ams6secmon25ReleaseCommonSmcStackLockEv

    /* Invoke the function with the new stack. */
    br   x19

.section    .text._ZN3ams6secmon3smc16FinalizePowerOffEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon3smc16FinalizePowerOffEv
_ZN3ams6secmon3smc16FinalizePowerOffEv:
    /* Disable all caches by clearing sctlr_el1.C. */
    mrs x0, sctlr_el1
    and x0, x0, #~(1 << 2)
    msr sctlr_el1, x0
    isb

    /* Disable all caches by clearing sctlr_el3.C. */
    mrs x0, sctlr_el3
    and x0, x0, #~(1 << 2)
    msr sctlr_el3, x0
    isb

    /* Disable prefetching of page table walking descriptors. */
    mrs x0, cpuectlr_el1
    orr x0, x0, #(1 << 38)

    /* Disable prefetching of instructions. */
    and x0, x0, #~(3 << 35)

    /* Disable prefetching of data. */
    and x0, x0, #~(3 << 32)
    msr cpuectlr_el1, x0
    isb

    /* Ensure that all data prefetching prior to our configuration change completes. */
    dsb sy

    /* Flush the entire data cache (local). */
    bl _ZN3ams6secmon25FlushEntireDataCacheLocalEv

    /* Disable receiving instruction cache/TLB maintenance operations. */
    mrs x0, cpuectlr_el1
    and x0, x0, #~(1 << 6)
    msr cpuectlr_el1, x0

    /* Configure the gic to not send interrupts to the current core. */
    ldr x1, =0x1F0043000
    mov w0, #0x1E0 /* Set FIQBypDisGrp1, IRQBypDisGrp1, reserved bits 7/8. */
    str w0, [x1]

    /* Lock the OS Double Lock. */
    mrs x0, osdlr_el1
    orr x0, x0, #(1 << 0)
    msr osdlr_el1, x0

    /* Ensure that our configuration takes. */
    isb
    dsb sy

    /* Wait for interrupts, infinitely. */
0:
    wfi
    b 0b



