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

#define LOAD_IMMEDIATE_32(reg, val)                \
    mov  reg, #(((val) >> 0x00) & 0xFFFF);         \
    movk reg, #(((val) >> 0x10) & 0xFFFF), lsl#16

.section    .crt0.text.start, "ax", %progbits
.global     _start
_start:
    b _main
__metadata_begin:
    .ascii "MLD0"                     /* Magic */
__metadata_target_firmware:
    .word  0xCCCCCCCC                 /* Target Firmware. */
__metadata_reserved:
    .word  0xCCCCCCCC                 /* Reserved. */
_main:
    /* KernelLdr_Main(uintptr_t kernel_base_address, KernelMap *kernel_map, uintptr_t ini1_base_address); */
    adr x18, _start
    adr x16, __external_references
    ldr x17, [x16, #0x8] /* bss end */
    ldr x16, [x16, #0x0] /* bss start */
    add x16, x16, x18
    add x17, x17, x18
    clear_bss:
        cmp x16, x17
        b.cs clear_bss_done
        str xzr, [x16],#0x8
        b clear_bss
    clear_bss_done:
    adr x17, __external_references
    ldr x17, [x17, #0x10] /* stack top */
    add sp, x17, x18

    /* Stack is now set up, so save important state. */
    sub sp, sp, #0x30
    stp x0, x1, [sp, #0x00]
    stp x2, x30, [sp, #0x10]
    stp xzr, xzr, [sp, #0x20]

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    /* Get the target firmware from exosphere. */
    LOAD_IMMEDIATE_32(w0, 0xC3000004)
    mov w1, #65000
    smc #1
    cmp x0, #0
0:
    b.ne 0b

    /* Store the target firmware. */
    adr x0, __metadata_target_firmware
    str w1, [x0]
#endif

    /* Apply relocations and call init array for KernelLdr. */
    adr x0, _start
    adr x1, __external_references
    ldr x1, [x1, #0x18] /* .dynamic. */
    add x1, x0, x1

    /* branch to ams::kern::init::Elf::ApplyRelocations(uintptr_t, const ams::kern::init::Elf::Elf64::Dyn *); */
    bl _ZN3ams4kern4init3Elf16ApplyRelocationsEmPKNS2_5Elf643DynE

    /* branch to ams::kern::init::Elf::CallInitArrayFuncs(uintptr_t, uintptr_t) */
    adr x2, _start
    adr x1, __external_references
    ldr x0, [x1, #0x20] /* init_array_start */
    ldr x1, [x1, #0x28] /* init_array_end */
    add x0, x0, x2
    add x1, x1, x2
    bl _ZN3ams4kern4init3Elf18CallInitArrayFuncsEmm

    /* Setup system registers, for detection of errors during init later. */
    msr tpidr_el1, xzr /* Clear TPIDR_EL1 */
    adr x0, __external_references
    adr x1, _start
    ldr x0, [x0,  #0x30]
    add x0, x1, x0
    msr vbar_el1, x0
    isb

    /* Call ams::kern::init::loader::Main(uintptr_t, ams::kern::init::KernelLayout *, uintptr_t) */
    ldp x0, x1, [sp, #0x00]
    ldr x2,     [sp, #0x10]

    bl _ZN3ams4kern4init6loader4MainEmPNS1_12KernelLayoutEm
    str x0, [sp, #0x00]

    /* Get ams::kern::init::loader::AllocateKernelInitStack(). */
    bl _ZN3ams4kern4init6loader23AllocateKernelInitStackEv
    str x0, [sp, #0x20]


    /* Call ams::kern::init::loader::GetFinalPageAllocatorState() */
    bl _ZN3ams4kern4init6loader26GetFinalPageAllocatorStateEv

    /* X0 is now the saved state for the page allocator. */
    /* We will return this to the kernel. */

    /* Return to the newly-relocated kernel. */
    ldr x1, [sp, #0x18] /* Return address to Kernel */
    ldr x2, [sp, #0x00] /* Relocated kernel base address diff. */
    add x1, x2, x1
    ldr x2, [sp, #0x20]
    mov sp, x2
    br  x1

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
.global     _ZN3ams4kern17GetTargetFirmwareEv
.type       _ZN3ams4kern17GetTargetFirmwareEv, %function
_ZN3ams4kern17GetTargetFirmwareEv:
    adr x0, __metadata_target_firmware
    ldr w0, [x0]
    ret
#endif

.balign 8
__external_references:
    .quad __bss_start__ - _start
    .quad __bss_end__   - _start
    .quad __stack_end - _start
    .quad _DYNAMIC - _start
    .quad __init_array_start - _start
    .quad __init_array_end   - _start
    .quad __vectors_start__  - _start