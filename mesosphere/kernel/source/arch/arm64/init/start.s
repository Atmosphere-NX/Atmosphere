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

#define LOAD_IMMEDIATE_64(reg, val)                \
    mov  reg, #(((val) >> 0x00) & 0xFFFF);         \
    movk reg, #(((val) >> 0x10) & 0xFFFF), lsl#16; \
    movk reg, #(((val) >> 0x20) & 0xFFFF), lsl#32; \
    movk reg, #(((val) >> 0x30) & 0xFFFF), lsl#48

#define LOAD_FROM_LABEL(reg, label) \
    adr reg, label;                 \
    ldr reg, [reg]

.section    .crt0.text.start, "ax", %progbits
.global     _start
_start:
    b _ZN3ams4kern4init10StartCore0Emm
__metadata_begin:
    .ascii "MSS0"                     /* Magic */
__metadata_ini_offset:
    .quad  0                          /* INI1 base address. */
__metadata_kernelldr_offset:
    .quad  0                          /* Kernel Loader base address. */
__metadata_target_firmware:
    .word  0xCCCCCCCC                 /* Target firmware. */
__metadata_kernel_layout:
    .word _start - _start             /* rx_offset */
    .word __rodata_start     - _start /* rx_end_offset */
    .word __rodata_start     - _start /* ro_offset */
    .word __data_start       - _start /* ro_end_offset */
    .word __data_start       - _start /* rw_offset */
    .word __bss_start__      - _start /* rw_end_offset */
    .word __bss_start__      - _start /* bss_offset */
    .word __bss_end__        - _start /* bss_end_offset */
    .word __end__            - _start /* ini_load_offset */
    .word _DYNAMIC           - _start /* dynamic_offset */
    .word __init_array_start - _start /* init_array_offset */
    .word __init_array_end   - _start /* init_array_end_offset */
.if (. - __metadata_begin) != 0x48
    .error "Incorrect Mesosphere Metadata"
.endif

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
.global     _ZN3ams4kern17GetTargetFirmwareEv
.type       _ZN3ams4kern17GetTargetFirmwareEv, %function
_ZN3ams4kern17GetTargetFirmwareEv:
    adr x0, __metadata_target_firmware
    ldr w0, [x0]
    ret
#endif

/* ams::kern::init::StartCore0(uintptr_t, uintptr_t) */
.section    .crt0.text._ZN3ams4kern4init10StartCore0Emm, "ax", %progbits
.global     _ZN3ams4kern4init10StartCore0Emm
.type       _ZN3ams4kern4init10StartCore0Emm, %function
_ZN3ams4kern4init10StartCore0Emm:
    /* Mask all interrupts. */
    msr daifset, #0xF

    /* Save arguments for later use. */
    mov x19, x0
    mov x20, x1

    /* Check our current EL. We want to be executing out of EL1. */
    /* If we're in EL2, we'll need to deprivilege ourselves. */
    mrs x1, currentel
    cmp x1, #0x4
    b.eq core0_el1
    cmp x1, #0x8
    b.eq core0_el2
core0_el3:
    b core0_el3
core0_el2:
    bl _ZN3ams4kern4init16JumpFromEL2ToEL1Ev
core0_el1:
    bl _ZN3ams4kern4init19DisableMmuAndCachesEv

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

    /* We want to invoke kernel loader. */
    adr x0, _start
    adr x1, __metadata_kernel_layout
    LOAD_FROM_LABEL(x2, __metadata_ini_offset)
    add x2, x0, x2
    LOAD_FROM_LABEL(x3, __metadata_kernelldr_offset)
    add x3, x0, x3

    /* Invoke kernel loader. */
    blr x3

    /* At this point kernelldr has been invoked, and we are relocated at a random virtual address. */
    /* Next thing to do is to set up our memory management and slabheaps -- all the other core initialization. */
    /* Call ams::kern::init::InitializeCore(uintptr_t, uintptr_t) */
    mov x1, x0  /* Kernelldr returns a KInitialPageAllocator state for the kernel to re-use. */
    mov x0, xzr /* Official kernel always passes zero, when this is non-zero the address is mapped. */
    bl _ZN3ams4kern4init14InitializeCoreEmm

    /* Get the init arguments for core 0. */
    mov x0, xzr
    bl _ZN3ams4kern4init23GetInitArgumentsAddressEi

    bl _ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE

/* ams::kern::init::StartOtherCore(const ams::kern::init::KInitArguments *) */
.section    .crt0.text._ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE, "ax", %progbits
.global     _ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE
.type       _ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE, %function
_ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE:
    /* Preserve the KInitArguments pointer in a register. */
    mov x20, x0

    /* Check our current EL. We want to be executing out of EL1. */
    /* If we're in EL2, we'll need to deprivilege ourselves. */
    mrs x1, currentel
    cmp x1, #0x4
    b.eq othercore_el1
    cmp x1, #0x8
    b.eq othercore_el2
othercore_el3:
    b othercore_el3
othercore_el2:
    bl _ZN3ams4kern4init16JumpFromEL2ToEL1Ev
othercore_el1:
    bl _ZN3ams4kern4init19DisableMmuAndCachesEv

    /* Setup system registers using values from our KInitArguments. */
    ldr x1, [x20, #0x00]
    msr ttbr0_el1, x1
    ldr x1, [x20, #0x08]
    msr ttbr1_el1, x1
    ldr x1, [x20, #0x10]
    msr tcr_el1, x1
    ldr x1, [x20, #0x18]
    msr mair_el1, x1

    /* Perform cpu-specific setup. */
    mrs x1, midr_el1
    ubfx x2, x1, #0x18, #0x8 /* Extract implementer bits. */
    cmp x2, #0x41            /* Implementer::ArmLimited */
    b.ne othercore_cpu_specific_setup_end
    ubfx x2, x1, #0x4, #0xC  /* Extract primary part number. */
    cmp x2, #0xD07           /* PrimaryPartNumber::CortexA57 */
    b.eq othercore_cpu_specific_setup_cortex_a57
    cmp x2, #0xD03           /* PrimaryPartNumber::CortexA53 */
    b.eq othercore_cpu_specific_setup_cortex_a53
    b othercore_cpu_specific_setup_end
othercore_cpu_specific_setup_cortex_a57:
othercore_cpu_specific_setup_cortex_a53:
    ldr x1, [x20, #0x20]
    msr cpuactlr_el1, x1
    ldr x1, [x20, #0x28]
    msr cpuectlr_el1, x1

othercore_cpu_specific_setup_end:
    /* Ensure instruction consistency. */
    dsb sy
    isb

    /* Set sctlr_el1 and ensure instruction consistency. */
    ldr x1, [x20, #0x30]
    msr sctlr_el1, x1

    dsb sy
    isb

    /* Jump to the virtual address equivalent to ams::kern::init::InvokeEntrypoint */
    ldr x1, [x20, #0x50]
    adr x2, _ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE
    sub x1, x1, x2
    adr x2, _ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE
    add x1, x1, x2
    mov x0, x20
    br x1

/* ams::kern::init::InvokeEntrypoint(const ams::kern::init::KInitArguments *) */
.section    .crt0.text._ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE, "ax", %progbits
.global     _ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE
.type       _ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE, %function
_ZN3ams4kern4init16InvokeEntrypointEPKNS1_14KInitArgumentsE:
    /* Preserve the KInitArguments pointer in a register. */
    mov x20, x0

    /* Clear CPACR_EL1. This will prevent classes of traps (SVE, etc). */
    msr cpacr_el1, xzr
    isb

    /* Setup the stack pointer. */
    ldr x1, [x20, #0x38]
    mov sp, x1

    /* Ensure that system debug registers are setup. */
    bl _ZN3ams4kern4init24InitializeDebugRegistersEv

    /* Ensure that the exception vectors are setup. */
    bl _ZN3ams4kern4init26InitializeExceptionVectorsEv

    /* Setup the exception stack in tpidr_el1. */
    ldr x1, [x20, #0x58]
    msr tpidr_el1, x1

    /* Jump to the entrypoint. */
    ldr x1, [x20, #0x40]
    ldr x0, [x20, #0x48]
    br x1


/* ams::kern::init::JumpFromEL2ToEL1() */
.section    .crt0.text._ZN3ams4kern4init16JumpFromEL2ToEL1Ev, "ax", %progbits
.global     _ZN3ams4kern4init16JumpFromEL2ToEL1Ev
.type       _ZN3ams4kern4init16JumpFromEL2ToEL1Ev, %function
_ZN3ams4kern4init16JumpFromEL2ToEL1Ev:
    /* We're going to want to ERET to our caller. */
    msr elr_el2, x30

    /* Flush the entire data cache and invalidate the entire TLB. */
    bl _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv

    /* Setup system registers for deprivileging. */

    /* Check if we're on cortex A57 or A53. If we are, set ACTLR_EL2. */
    mrs  x1, midr_el1

    /* Is the manufacturer ID 'A' (ARM)? */
    ubfx x2, x1, #0x18, #8
    cmp x2, #0x41
    b.ne 2f

    /* Is the board ID Cortex-A57? */
    ubfx x2, x1, #4, #0xC
    mov x3, #0xD07
    cmp x2, x3
    b.eq 1f

    /* Is the board ID Cortex-A53? */
    mov x3, #0xD03
    cmp x2, x3
    b.ne 2f

1:
    /* ACTLR_EL2: */
    /*  - CPUACTLR access control = 1 */
    /*  - CPUECTLR access control = 1 */
    /*  - L2CTLR access control = 1 */
    /*  - L2ECTLR access control = 1 */
    /*  - L2ACTLR access control = 1 */
    mov x0, #0x73
    msr actlr_el2, x0

2:
    /* HCR_EL2: */
    /*  - RW = 1 (el1 is aarch64) */
    mov x0, #0x80000000
    msr hcr_el2, x0

    /* SCTLR_EL1: */
    /*  - EOS = 1 */
    /*  - EIS = 1 */
    /*  - SPAN = 1 */
    LOAD_IMMEDIATE_32(x0, 0x00C00800)
    msr sctlr_el1, x0

    /* DACR32_EL2: */
    /*  - Manager access for all D<n> */
    mov x0, #0xFFFFFFFF
    msr dacr32_el2, x0

    /* Set VPIDR_EL2 = MIDR_EL1 */
    mrs x0, midr_el1
    msr vpidr_el2, x0

    /* SET VMPIDR_EL2 = MPIDR_EL1 */
    mrs x0, mpidr_el1
    msr vmpidr_el2, x0

    /* SPSR_EL2: */
    /*  - EL1h */
    /*  - IRQ masked */
    /*  - FIQ masked */
    mov x0, #0xC5
    msr spsr_el2, x0

    eret

/* ams::kern::init::DisableMmuAndCaches() */
.section    .crt0.text._ZN3ams4kern4init19DisableMmuAndCachesEv, "ax", %progbits
.global     _ZN3ams4kern4init19DisableMmuAndCachesEv
.type       _ZN3ams4kern4init19DisableMmuAndCachesEv, %function
_ZN3ams4kern4init19DisableMmuAndCachesEv:
    /* The stack isn't set up, so we'll need to trash a register. */
    mov x22, x30

    /* Flush the entire data cache and invalidate the entire TLB. */
    bl _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv

    /* Invalidate the instruction cache, and ensure instruction consistency. */
    ic ialluis
    dsb sy
    isb

    /* Set SCTLR_EL1 to disable the caches and mmu. */
    /* SCTLR_EL1: */
    /*  - M = 0 */
    /*  - C = 0 */
    /*  - I = 0 */
    mrs x0, sctlr_el1
    LOAD_IMMEDIATE_64(x1, ~0x1005)
    and x0, x0, x1
    msr sctlr_el1, x0

    mov x30, x22
    ret

/* ams::kern::arch::arm64::cpu::FlushEntireDataCacheWithoutStack() */
.section    .crt0.text._ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv
.type       _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv, %function
_ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv:
    /* The stack isn't set up, so we'll need to trash a register. */
    mov x23, x30

    /* Ensure that the cache is coherent. */
    bl _ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv
    dsb sy

    bl _ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv
    dsb sy

    bl _ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv
    dsb sy

    /* Invalidate the entire TLB, and ensure instruction consistency. */
    tlbi vmalle1is
    dsb sy
    isb

    mov x30, x23
    ret

/* ams::kern::arch::arm64::cpu::FlushEntireDataCacheLocalWithoutStack() */
.section    .crt0.text._ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv
.type       _ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv, %function
_ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv:
    /* The stack isn't set up, so we'll need to trash a register. */
    mov x24, x30

    /* CacheLineIdAccessor clidr_el1; */
    mrs x10, clidr_el1
    /* const int levels_of_unification = clidr_el1.GetLevelsOfUnification(); */
    ubfx x10, x10, #0x15, 3

    /* int level = levels_of_unification - 1 */
    sub w9, w10, #1

    /* while (level >= 0) { */
begin_flush_cache_local_loop:
    cmn w9, #1
    b.eq done_flush_cache_local_loop

    /*     FlushEntireDataCacheImplWithoutStack(level); */
    mov w0, w9
    bl _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv

    /*     level--; */
    sub w9, w9, #1

    /* } */
    b begin_flush_cache_local_loop

done_flush_cache_local_loop:
    mov x30, x24
    ret

/* ams::kern::arch::arm64::cpu::FlushEntireDataCacheSharedWithoutStack() */
.section    .crt0.text._ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv
.type       _ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv, %function
_ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv:
    /* The stack isn't set up, so we'll need to trash a register. */
    mov x24, x30

    /* CacheLineIdAccessor clidr_el1; */
    mrs x10, clidr_el1
    /* const int levels_of_coherency   = clidr_el1.GetLevelsOfCoherency(); */
    ubfx x9, x10,  #0x18, 3
    /* const int levels_of_unification = clidr_el1.GetLevelsOfUnification(); */
    ubfx x10, x10, #0x15, 3

    /* int level = levels_of_coherency */

    /* while (level >= levels_of_unification) { */
begin_flush_cache_shared_loop:
    cmp w10, w9
    b.gt done_flush_cache_shared_loop

    /*     FlushEntireDataCacheImplWithoutStack(level); */
    mov w0, w9
    bl _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv

    /*     level--; */
    sub w9, w9, #1

    /* } */
    b begin_flush_cache_shared_loop

done_flush_cache_shared_loop:
    mov x30, x24
    ret

/* ams::kern::arch::arm64::cpu::FlushEntireDataCacheImplWithoutStack() */
.section    .crt0.text._ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv
.type       _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv, %function
_ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv:
    /* const u64 level_sel_value = static_cast<u64>(level << 1); */
    lsl w6, w0, #1
    sxtw x6, w6

    /* cpu::SetCsselrEl1(level_sel_value); */
    msr csselr_el1, x6

    /* cpu::InstructionMemoryBarrier(); */
    isb

    /* CacheSizeIdAccessor ccsidr_el1; */
    mrs x3, ccsidr_el1

    /* const int num_ways  = ccsidr_el1.GetAssociativity(); */
    ubfx x7, x3, #3, #0xA
    mov w8, w7

    /* const int line_size = ccsidr_el1.GetLineSize(); */
    and x4, x3, #7

    /* const u64 way_shift = static_cast<u64>(__builtin_clz(num_ways)); */
    clz w7, w7

    /* const u64 set_shift = static_cast<u64>(line_size + 4); */
    add w4, w4, #4

    /* const int num_sets  = ccsidr_el1.GetNumberOfSets(); */
    ubfx w3, w3, #0xD, #0xF

    /* int way = 0; */
    mov x5, #0

    /* while (way <= num_ways) { */
begin_flush_cache_impl_way_loop:
    cmp w8, w5
    b.lt done_flush_cache_impl_way_loop

    /*     int set = 0; */
    mov x0, #0

    /*     while (set <= num_sets) { */
begin_flush_cache_impl_set_loop:
    cmp w3, w0
    b.lt done_flush_cache_impl_set_loop

    /*         const u64 cisw_value = (static_cast<u64>(way) << way_shift) | (static_cast<u64>(set) << set_shift) | level_sel_value; */
    lsl x2, x5, x7
    lsl x1, x0, x4
    orr x1, x1, x2
    orr x1, x1, x6

    /*         __asm__ __volatile__("dc cisw, %0" :: "r"(cisw_value) : "memory"); */
    dc cisw, x1

    /*         set++; */
    add x0, x0, #1

    /*     } */
    b begin_flush_cache_impl_set_loop
done_flush_cache_impl_set_loop:

    /*     way++; */
    add x5, x5, 1

    /* } */
    b begin_flush_cache_impl_way_loop
done_flush_cache_impl_way_loop:
    ret
