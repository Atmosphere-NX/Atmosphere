/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "asm_macros.s"

.section    .crt0, "ax", %progbits
.align      3
.global     _start
.type       _start, %function

_start:
    b       _start1
    b       _start2

.global _ZN3ams6hvisor11CoreContext23initialKernelEntrypointE
_ZN3ams6hvisor11CoreContext23initialKernelEntrypointE:
    .quad   0

_start1:
    mov     x19, #1
    b       _startCommon
_start2:
    mov     x19, xzr
_startCommon:
    // Disable interrupts, select sp_el0 before mmu is enabled
    mrs     x20, cntpct_el0
    msr     daifset, 0b1111
    msr     spsel, #0

    // Set sctlr_el2 ASAP to disable mmu/caching if not already done.
    mov     x1, #0x0838
    movk    x1, #0x30C5,lsl #16
    msr     sctlr_el2, x1
    dsb     sy
    isb

    // Save x0
    mov     x21, x0

    // Get core ID
    mrs     x22, mpidr_el1
    and     x22, x22, #0xFF

    // ams::hvisor::cpu::ClearLocalDataCacheOnBoot
    bl      _ZN3ams6hvisor3cpu25ClearLocalDataCacheOnBootEv
    cbz     x19, 1f

    // "Boot core only" stuff:
    // ams::hvisor::cpu::ClearSharedDataCachesOnBoot
    bl      _ZN3ams6hvisor3cpu27ClearSharedDataCachesOnBootEv
    ic      iallu
    dsb     sy
    isb

    // Temporarily use temp end region as stack, then create the translation table
    // The stack top is also equal to the mmu table address...
    adr     x0, _ZN3ams6hvisor9MemoryMap11imageLayoutE
    mov     sp, x1
    // ams::hvisor::MemoryMap::SetupMmu(ams::hvisor::MemoryMap::LoadImageLayout const*)
    bl      _ZN3ams6hvisor9MemoryMap8SetupMmuEPKNS1_15LoadImageLayoutE

1:
    // Enable MMU, note that the function is not allowed to use any stack
    adr     x0, _ZN3ams6hvisor9MemoryMap11imageLayoutE
    mov     w1, w22
    ldr     x18, =_postMmuEnableReturnAddr
    // ams::hvisor::MemoryMap::EnableMmuGetStacks(ams::hvisor::MemoryMap::LoadImageLayout const*, unsigned int)
    bl      _ZN3ams6hvisor9MemoryMap18EnableMmuGetStacksEPKNS1_15LoadImageLayoutEj

    // This is where we will land on exception return after enabling the MMU:
_postMmuEnableReturnAddr:
    // x0 = sp, x1 = crash sp

    mov     x23, x1

    // Select sp_el2
    msr     spsel, #1

    mov     sp, x0
    msr     sp_el0, x23

    // Set up x18, other sysregs, BSS, etc.
    // Don't call init array to save space?
    mov     w0, w22
    mov     w1, w19
    mov     x2, x21
    bl      initSystem

    // Save x18, reserve space for exception frame
    stp     x18, x23, [sp, #-0x10]!
    sub     sp, sp, #EXCEP_STACK_FRAME_SIZE

    prfm    pstl1keep, [x18]

    mov     x0, sp
    mov     x1, x20
    //str     x0, [x18, #CORECTX_GUEST_FRAME_OFFSET]
    bl      thermosphereMain

    dsb     sy
    isb

    // Jump to kernel
    mov     x0, sp
    // ams::hvisor::ExceptionEntryPostprocess(ams::hvisor::ExceptionStackFrame*, bool)
    bl      _ZN3ams6hvisor25ExceptionEntryPostprocessEPNS0_19ExceptionStackFrameEb
    b       _restoreAllRegisters

.pool

// ams::hvisor::MemoryMap::imageLayout
.global     _ZN3ams6hvisor9MemoryMap11imageLayoutE
_ZN3ams6hvisor9MemoryMap11imageLayoutE:
    .quad       __start_pa__
    .quad       __image_size__
    .quad       __temp_pa__
    .quad       __max_temp_size__
    .quad       __temp_size__
    .quad       __vectors_start__
