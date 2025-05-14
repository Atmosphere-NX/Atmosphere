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
#include <mesosphere/kern_select_assembly_offsets.h>

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

#define LOAD_RELATIVE_FROM_LABEL(reg, reg2, label) \
    adr reg2, label;                               \
    ldr reg, [reg2];                               \
    add reg, reg, reg2

#define INDIRECT_RELATIVE_CALL(reg, reg2, label) \
    adr reg, label;                              \
    add reg, reg, reg2;                          \
    blr reg

#define SETUP_SYSTEM_REGISTER(_reg, _sr)              \
    LOAD_FROM_LABEL(_reg, __sysreg_constant_ ## _sr); \
    msr _sr, _reg

.section    .start, "ax", %progbits
.global     _start
_start:
    b _ZN3ams4kern4init10StartCore0Emm
__metadata_magic_number:
    .ascii "MSS1"                     /* Magic, if executed as gadget "adds w13, w26, #0x4d4, lsl #12" */
__metadata_offset:
    .word __metadata_begin - _start

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
.global     _ZN3ams4kern17GetTargetFirmwareEv
.type       _ZN3ams4kern17GetTargetFirmwareEv, %function
_ZN3ams4kern17GetTargetFirmwareEv:
    adr x0, __metadata_target_firmware
    ldr w0, [x0]
    ret
#endif

.section    .crt0.text.start, "ax", %progbits

/* ams::kern::init::IdentityMappedFunctionAreaBegin() */
.global     _ZN3ams4kern4init31IdentityMappedFunctionAreaBeginEv
.type       _ZN3ams4kern4init31IdentityMappedFunctionAreaBeginEv, %function
_ZN3ams4kern4init31IdentityMappedFunctionAreaBeginEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */

/*  ================ Functions after this line remain identity-mapped after initialization finishes. ================ */

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
    mrs x1, currentel

    /* Check if we're EL1. */
    cmp x1, #0x4
    b.eq 2f

    /* Check if we're EL2. */
    cmp x1, #0x8
    b.eq 1f

0:  /* We're EL3. This is a panic condition. */
    b 0b

1:  /* We're EL2. */
    #ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    /* On NX board, this is a panic condition. */
    b 1b
    #else
    /* Otherwise, deprivilege to EL2. */
    /* TODO: Does N still have this? We need it for qemu emulation/unit testing, we should come up with a better solution maybe. */
    bl _ZN3ams4kern4init16JumpFromEL2ToEL1Ev
    #endif

2:  /* We're EL1. */
    /* Flush the entire data cache and invalidate the entire TLB. */
    bl _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv

    /* Invalidate the instruction cache, and ensure instruction consistency. */
    ic ialluis
    dsb sy
    isb

    /* Disable the MMU/Caches. */
    bl _ZN3ams4kern4init19DisableMmuAndCachesEv

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    /* Get the target firmware from exosphere. */
    LOAD_IMMEDIATE_32(w0, 0xC3000004)
    mov w1, #65000
    smc #1
    cmp x0, #0
3:
    b.ne 3b

    /* Store the target firmware. */
    adr x0, __metadata_target_firmware
    str w1, [x0]
#endif

    /* Get the unknown debug region. */
    /* TODO: This is always zero in release kernels -- what is this? Is it the device tree buffer? */
    mov x21, #0
    nop

    /* We want to invoke kernel loader. */
    adr x0, _start
    adr x1, __metadata_kernel_layout
    LOAD_RELATIVE_FROM_LABEL(x2, x4, __metadata_ini_offset)
    LOAD_RELATIVE_FROM_LABEL(x3, x4, __metadata_kernelldr_offset)

    /* Invoke kernel loader. */
    blr x3

    /* Save the offset to virtual address from this page's physical address for our use. */
    mov x24, x1

    /* Clear the platform register (used for Kernel::GetCurrentThreadPointer()) */
    mov x18, #0

    /* At this point kernelldr has been invoked, and we are relocated at a random virtual address. */
    /* Next thing to do is to set up our memory management and slabheaps -- all the other core initialization. */
    /* Call ams::kern::init::InitializeCore(uintptr_t, void **) */
    mov x1, x0  /* Kernelldr returns a state object for the kernel to re-use. */
    mov x0, x21 /* Use the address we determined earlier. */
    nop
    INDIRECT_RELATIVE_CALL(x16, x24, _ZN3ams4kern4init20InitializeCorePhase1EmPPv)

    /* Get the init arguments for core 0. */
    mov x0, xzr
    nop
    INDIRECT_RELATIVE_CALL(x16, x24, _ZN3ams4kern4init16GetInitArgumentsEi)

    /* Setup the stack pointer. */
    ldr x2, [x0, #(INIT_ARGUMENTS_SP)]
    mov sp, x2

    /* Perform further initialization with the stack pointer set up, as required. */
    /* This will include e.g. unmapping the identity mapping. */
    nop
    INDIRECT_RELATIVE_CALL(x16, x24, _ZN3ams4kern4init20InitializeCorePhase2Ev)

    /* Get the init arguments for core 0. */
    mov x0, xzr
    nop
    INDIRECT_RELATIVE_CALL(x16, x24, _ZN3ams4kern4init16GetInitArgumentsEi)

    /* Retrieve entrypoint and argument. */
    ldr x1, [x0, #(INIT_ARGUMENTS_ENTRYPOINT)]
    ldr x0, [x0, #(INIT_ARGUMENTS_ARGUMENT)]

    /* Set sctlr_el1 and ensure instruction consistency. */
    SETUP_SYSTEM_REGISTER(x3, sctlr_el1)

    dsb sy
    isb

    /* Invoke the entrypoint. */
    blr x1

0:  /* If we return here, something has gone wrong, so wait forever. */
    b 0b

/* ams::kern::init::StartOtherCore(const ams::kern::init::KInitArguments *) */
.section    .crt0.text._ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE, "ax", %progbits
.global     _ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE
.type       _ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE, %function
_ZN3ams4kern4init14StartOtherCoreEPKNS1_14KInitArgumentsE:
    /* Preserve the KInitArguments pointer in a register. */
    mov x20, x0

    /* Check our current EL. We want to be executing out of EL1. */
    mrs x1, currentel

    /* Check if we're EL1. */
    cmp x1, #0x4
    b.eq 2f

    /* Check if we're EL2. */
    cmp x1, #0x8
    b.eq 1f

0:  /* We're EL3. This is a panic condition. */
    b 0b

1:  /* We're EL2. */
    #ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    /* On NX board, this is a panic condition. */
    b 1b
    #else
    /* Otherwise, deprivilege to EL2. */
    /* TODO: Does N still have this? We need it for qemu emulation/unit testing, we should come up with a better solution maybe. */
    bl _ZN3ams4kern4init16JumpFromEL2ToEL1Ev
    #endif

2:  /* We're EL1. */
    /* Flush the entire data cache and invalidate the entire TLB. */
    bl _ZN3ams4kern4arch5arm643cpu32FlushEntireDataCacheWithoutStackEv

    /* Invalidate the instruction cache, and ensure instruction consistency. */
    ic ialluis
    dsb sy
    isb

    /* Disable the MMU/Caches. */
    bl _ZN3ams4kern4init19DisableMmuAndCachesEv

    /* Setup system registers using values from constants table. */
    SETUP_SYSTEM_REGISTER(x1, ttbr0_el1)
    SETUP_SYSTEM_REGISTER(x1, ttbr1_el1)
    SETUP_SYSTEM_REGISTER(x1, tcr_el1)
    SETUP_SYSTEM_REGISTER(x1, mair_el1)

    /* Perform cpu-specific setup. */
    mrs x1, midr_el1
    ubfx x2, x1, #0x18, #0x8 /* Extract implementer bits. */
    cmp x2, #0x41            /* Implementer::ArmLimited */
    b.ne 4f
    ubfx x2, x1, #0x4, #0xC  /* Extract primary part number. */
    cmp x2, #0xD07           /* PrimaryPartNumber::CortexA57 */
    b.eq 3f
    cmp x2, #0xD03           /* PrimaryPartNumber::CortexA53 */
    b.eq 3f
    b 4f
3:  /* We're running on a Cortex-A53/Cortex-A57. */
    /* NOTE: Nintendo compares these values instead of setting them, infinite looping on incorrect value. */
    ldr x1, [x20, #(INIT_ARGUMENTS_CPUACTLR)]
    msr cpuactlr_el1, x1
    ldr x1, [x20, #(INIT_ARGUMENTS_CPUECTLR)]
    msr cpuectlr_el1, x1

4:
    /* Ensure instruction consistency. */
    dsb sy
    isb

    /* Load remaining needed fields from the init args. */
    ldr x2, [x20, #(INIT_ARGUMENTS_SP)]
    ldr x1, [x20, #(INIT_ARGUMENTS_ENTRYPOINT)]
    ldr x0, [x20, #(INIT_ARGUMENTS_ARGUMENT)]

    /* Set sctlr_el1 and ensure instruction consistency. */
    SETUP_SYSTEM_REGISTER(x3, sctlr_el1)

    dsb sy
    isb

    /* Set the stack pointer. */
    mov sp, x2

    /* Clear the platform register (used for Kernel::GetCurrentThreadPointer()) */
    mov x18, #0

    /* Invoke the entrypoint. */
    blr x1

0:  /* If we return here, something has gone wrong, so wait forever. */
    b 0b

/* Nintendo places the metadata after StartOthercore. */
.align 8

__metadata_begin:
__metadata_ini_offset:
    .quad  0                          /* INI1 base address. */
__metadata_kernelldr_offset:
    .quad  0                          /* Kernel Loader base address. */
__metadata_target_firmware:
    .word  0xCCCCCCCC                 /* Target firmware. */
__metadata_kernel_layout:
    .word _start                  - __metadata_kernel_layout /* rx_offset */
    .word __rodata_start          - __metadata_kernel_layout /* rx_end_offset */
    .word __rodata_start          - __metadata_kernel_layout /* ro_offset */
    .word __data_start            - __metadata_kernel_layout /* ro_end_offset */
    .word __data_start            - __metadata_kernel_layout /* rw_offset */
    .word __bss_start__           - __metadata_kernel_layout /* rw_end_offset */
    .word __bss_start__           - __metadata_kernel_layout /* bss_offset */
    .word __bss_end__             - __metadata_kernel_layout /* bss_end_offset */
    .word __end__                 - __metadata_kernel_layout /* resource_offset */
    .word _DYNAMIC                - __metadata_kernel_layout /* dynamic_offset */
    .word __init_array_start      - __metadata_kernel_layout /* init_array_offset */
    .word __init_array_end        - __metadata_kernel_layout /* init_array_end_offset */
    .word __sysreg_constant_begin - __metadata_kernel_layout /* sysreg_offset */
.if (. - __metadata_begin) != 0x48
    .error "Incorrect Mesosphere Metadata"
.endif


/* TODO: Can we remove this while retaining QEMU support? */
#ifndef ATMOSPHERE_BOARD_NINTENDO_NX
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

    ERET_WITH_SPECULATION_BARRIER
#endif

/* ams::kern::init::DisableMmuAndCaches() */
.section    .crt0.text._ZN3ams4kern4init19DisableMmuAndCachesEv, "ax", %progbits
.global     _ZN3ams4kern4init19DisableMmuAndCachesEv
.type       _ZN3ams4kern4init19DisableMmuAndCachesEv, %function
_ZN3ams4kern4init19DisableMmuAndCachesEv:
    /* The stack isn't set up, so we'll need to trash a register. */
    mov x22, x30

    /* Set SCTLR_EL1 to disable the caches and mmu. */
    /* SCTLR_EL1: */
    /*  - M = 0 */
    /*  - C = 0 */
    /*  - I = 0 */
    mrs x0, sctlr_el1
    LOAD_IMMEDIATE_64(x1, ~0x1005)
    and x0, x0, x1
    msr sctlr_el1, x0

    /* Ensure instruction consistency. */
    dsb sy
    isb

    mov x30, x22
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
    ubfx x9, x10,  #0x15, 3
    /* const int levels_of_unification = clidr_el1.GetLevelsOfUnification(); */
    ubfx x10, x10, #0x18, 3

    /* int level = levels_of_unification */

    /* while (level <= levels_of_coherency) { */
    cmp w9, w10
    b.hi 1f

0:
    /*     FlushEntireDataCacheImplWithoutStack(level); */
    mov w0, w9
    bl _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv

    /*     level++; */
    cmp w9, w10
    add w9, w9, #1

    /* } */
    b.cc 0b

    /* cpu::DataSynchronizationBarrier(); */
    dsb sy

1:
    mov x30, x24
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

    /* int level = 0 */
    mov x9, xzr

    /* while (level <= levels_of_unification) { */
    cmp x9, x10
    b.eq 1f

0:
    /*     FlushEntireDataCacheImplWithoutStack(level); */
    mov w0, w9
    bl _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv

    /*     level++; */
    add w9, w9, #1

    /* } */
    cmp x9, x10
    b.ne 0b

    /* cpu::DataSynchronizationBarrier(); */
    dsb sy

1:
    mov x30, x24
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

    bl _ZN3ams4kern4arch5arm643cpu38FlushEntireDataCacheSharedWithoutStackEv

    bl _ZN3ams4kern4arch5arm643cpu37FlushEntireDataCacheLocalWithoutStackEv

    /* Invalidate the entire TLB, and ensure instruction consistency. */
    tlbi vmalle1is
    dsb sy
    isb

    mov x30, x23
    ret

/* ams::kern::arch::arm64::cpu::FlushEntireDataCacheImplWithoutStack() */
.section    .crt0.text._ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv
.type       _ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv, %function
_ZN3ams4kern4arch5arm643cpu36FlushEntireDataCacheImplWithoutStackEv:
    /* const u64 level_sel_value = static_cast<u64>(level << 1); */
    lsl w6, w0, #1
    sxtw x6, w6

    /* cpu::DataSynchronizationBarrier(); */
    dsb sy

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
0:
    cmp w8, w5
    b.lt 3f

    /*     int set = 0; */
    mov x0, #0

    /*     while (set <= num_sets) { */
1:
    cmp w3, w0
    b.lt 2f

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
    b 1b
2:

    /*     way++; */
    add x5, x5, 1

    /* } */
    b 0b
3:
    ret


/* System register values. */
.align 8

__sysreg_constant_begin:
__sysreg_constant_ttbr0_el1:
    .quad  0 /* ttbr0_e11. */
__sysreg_constant_ttbr1_el1:
    .quad  0 /* ttbr1_e11. */
__sysreg_constant_tcr_el1:
    .quad  0 /* tcr_e11. */
__sysreg_constant_mair_el1:
    .quad  0 /* mair_e11. */
__sysreg_constant_sctlr_el1:
    .quad  0 /* sctlr_e11. */
.if (. - __sysreg_constant_begin) != 0x28
    .error "Incorrect System Registers"
.endif

/*  ================ Functions before this line remain identity-mapped after initialization finishes. ================ */

/* ams::kern::init::IdentityMappedFunctionAreaEnd() */
.global     _ZN3ams4kern4init29IdentityMappedFunctionAreaEndEv
.type       _ZN3ams4kern4init31IdentityMappedFunctionAreaEndEv, %function
_ZN3ams4kern4init29IdentityMappedFunctionAreaEndEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */