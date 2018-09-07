/*
 * Copyright (c) 2018 Atmosphère-NX
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

    RESET_CORE

.rept 65
    nop                     /* guard against speculative excecution */
.endr

    2:
    /* set the OS lock */
    mov  x0, #1
    msr  oslar_el1, x0
.endm

.section    .cold_crt0.text.start, "ax", %progbits
.align      6
.global     __start_cold
__start_cold:
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    /*
        This coldboot crt0 doesn't enter the boot critical section in the official monitor.
        However we'll initialize g_boot_critical_section so that it acts like core0 has entered it,
        for it to be in .data and for safety.
    */

    /* Relocate the crt0. Nintendo doesn't do it. */
    ldr  x0, =__cold_crt0_start__
    adr  x1, __start_cold
    ldr  x2, =__cold_crt0_end__
    cmp  x0, x1
    beq  _post_cold_crt0_reloc
    1:
        ldp  x3, x4, [x1], #0x10
        stp  x3, x4, [x0], #0x10
        cmp  x0, x2
        blo  1b
    
    adr  x19, __start_cold
    adr x20, g_coldboot_crt0_relocation_list
    sub x20, x20, x19
    ldr  x16, =_post_cold_crt0_reloc
    br   x16

_post_cold_crt0_reloc:
    /* Setup stack for coldboot crt0. */
    msr  spsel, #0
    bl   get_coldboot_crt0_temp_stack_address
    mov  sp, x0
    mov  fp, #0
    bl   get_coldboot_crt0_stack_address
    mov  sp, x0
    mov  fp, #0
    
    /* Relocate Exosphere image to free DRAM, clearing the image in IRAM. */
    ldr x0, =0x80010000
    add x20, x20, x0
    ldr x2, =__loaded_end_lma__
    ldr x3, =__glob_origin__
    sub x21, x2, x3
    mov x1, x19
    mov x2, x21
    add x2, x2, x0
    2:
        ldp x3, x4, [x1]
        stp x3, x4, [x0], #0x10
        stp xzr, xzr, [x1], #0x10
        cmp x0, x2
        blo 2b

    /* X0 = TZ-in-DRAM, X1 = relocation-list-in-DRAM. */
    mov  x0, x20
    ldr x1, =0x80010000
    /* Set size in coldboot relocation list. */
    str x21, [x0, #0x8]
    
    bl   coldboot_init

    ldr  x16, =__jump_to_main_cold
    br   x16

.section    .warm_crt0.text.start, "ax", %progbits
.align      6
.global     __start_warm
__start_warm:
    ERRATUM_INVALIDATE_BTB_AT_BOOT

    /* For some reasons, Nintendo uses spsel, #1 around here, causing issues if an exception occurs */
    msr  spsel, #0

    /* Nintendo doesn't use anything like the following two lines, but their critical section algo is borked */
    /* FWIW this function doesn't use a stack atm, with latest GCC, but that might change. */
    bl   get_warmboot_crt0_stack_address_critsec_enter
    mov  sp, x0

    /* PA(__main_start__) = __warmboot_crt0_start__ + 0x800 (refer to the linker script) */
    ldr  x0, =g_boot_critical_section
    bl   warmboot_crt0_critical_section_enter

    bl   get_warmboot_crt0_stack_address
    mov  sp, x0
    mov  fp, #0

    bl   warmboot_init
    ldr  x16, =__jump_to_main_warm
    br   x16

/* Used by coldboot as well */
.section    .warm_crt0.text.__set_memory_registers, "ax", %progbits
.align      4
.global     __set_memory_registers
.type       __set_memory_registers, %function
__set_memory_registers:
    msr  cpuectlr_el1, x2
    isb
    msr  scr_el3, x3
    msr  ttbr0_el3, x0
    msr  tcr_el3, x4
    msr  cptr_el3, x5
    msr  mair_el3, x6
    msr  vbar_el3, x1

    /* Invalidate the entire TLB on the Inner Shareable domain */
    isb
    dsb  ish
    tlbi alle3is
    dsb  ish
    isb

    msr  sctlr_el3, x7
    isb
    ret

.section    .text.__jump_to_main_cold, "ax", %progbits
.align      4
__jump_to_main_cold:
    /* This is inspired by Nintendo's code but significantly different */
    bl   __set_exception_entry_stack_pointer
    /*
        Normally Nintendo calls it in crt0, but it's fine to do that here.
        Please note that package2.c shouldn't have constructed objects, because we
        call __libc_fini_array after load_package2 has been cleared, on EL3
        to EL3 chainload.
    */
    bl   __libc_init_array

    bl   get_pk2ldr_stack_address
    mov  sp, x0

    mov  x0, x20
    bl   load_package2

    mov  w0, #3 /* use core3 stack temporarily */
    bl   get_exception_entry_stack_address
    mov  sp, x0
    bl   coldboot_main
    /* If we ever return, it's to chainload an EL3 payload */
    bl   __libc_fini_array
    /* Reset the core (only one is running on coldboot) */
    RESET_CORE

.section    .text.__jump_to_main_warm, "ax", %progbits
__jump_to_main_warm:
    /* Nintendo doesn't do that here, causing issues if an exception occurs */
    bl   __set_exception_entry_stack_pointer

    mov  w0, #0 /* use core0,1,2 stack bottom + 0x800 (VA of warmboot crt0 sp) temporarily */
    bl   get_warmboot_main_stack_address
    mov  sp, x0
    bl   warmboot_main

.section    .text.__set_exception_entry_stack, "ax", %progbits
.type       __set_exception_entry_stack, %function
.global     __set_exception_entry_stack
__set_exception_entry_stack_pointer:
    /* If SPSel == 1 on entry, make sure your function doesn't use stack variables! */
    mov  x16, lr
    mrs  x17, spsel
    mrs  x0, mpidr_el1
    and  w0, w0, #3
    bl   get_exception_entry_stack_address
    msr  spsel, #1
    mov  sp, x0
    msr  spsel, x17
    mov  lr, x16
    ret

.section    .text.__jump_to_lower_el, "ax", %progbits
.global     __jump_to_lower_el
.type       __jump_to_lower_el, %function
__jump_to_lower_el:
    /* x0: arg (context ID), x1: entrypoint, w2: spsr */
    mov x19, x0
    mov  w2, w2

    msr  elr_el3, x1
    msr  spsr_el3, x2

    bl __set_exception_entry_stack_pointer
    mov x0, x19

    isb
    eret

/* Custom stuff */
.section    .cold_crt0.data.g_coldboot_crt0_relocation_list, "aw", %progbits
.align      3
.global     g_coldboot_crt0_relocation_list
g_coldboot_crt0_relocation_list:
    .quad   0, 0  /* __start_cold, to be set & loaded size */
    .quad   1, 5                   /* number of sections to relocate/clear before & after mmu init */
    /* Relocations */
    .quad   __warmboot_crt0_start__, __warmboot_crt0_end__, __warmboot_crt0_lma__
    .quad   __main_start__, __main_bss_start__, __main_lma__
    .quad   __pk2ldr_start__, __pk2ldr_bss_start__, __pk2ldr_lma__
    .quad   __vectors_start__, __vectors_end__, __vectors_lma__
    /* BSS clears */
    .quad   __main_bss_start__, __main_end__, 0
    .quad   __pk2ldr_bss_start__, __pk2ldr_end__, 0

/* Critical section */
.section    .warm_crt0.data.g_boot_critical_section, "aw", %progbits
.align      2
.global     g_boot_critical_section
g_boot_critical_section:
    .word   1 /* Core0 entered, by default. */
