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
 
#define cpuactlr_el1 s3_1_c15_c2_0
#define cpuectlr_el1 s3_1_c15_c2_1

.section    .text.tlb_invalidate_all, "ax", %progbits
.type       tlb_invalidate_all, %function
.global     tlb_invalidate_all
tlb_invalidate_all:
    dsb  sy
    tlbi alle3
    dsb  sy
    isb
    ret

.section    .text.tlb_invalidate_all_inner_shareable, "ax", %progbits
.type       tlb_invalidate_all_inner_shareable, %function
.global     tlb_invalidate_all_inner_shareable
tlb_invalidate_all_inner_shareable:
    dsb  ish
    tlbi alle3is
    dsb  ish
    isb
    ret

.section    .text.tlb_invalidate_page, "ax", %progbits
.type       tlb_invalidate_page, %function
.global     tlb_invalidate_page
tlb_invalidate_page:
    lsr  x8, x0, #12
    dsb  sy
    tlbi vale3, x8
    dsb  sy
    isb
    ret

.section    .text.tlb_invalidate_page_inner_shareable, "ax", %progbits
.type       tlb_invalidate_page_inner_shareable, %function
.global     tlb_invalidate_page_inner_shareable
tlb_invalidate_page_inner_shareable:
    lsr  x8, x0, #12
    dsb  ish
    tlbi vale3is, x8
    dsb  ish
    isb
    ret

/* The following functions are taken/adapted from https://github.com/u-boot/u-boot/blob/master/arch/arm/cpu/armv8/cache.S */

/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * This file is based on sample code from ARMv8 ARM.
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */

/*
 * void __asm_dcache_level(level)
 *
 * flush or invalidate one level cache.
 *
 * x0: cache level
 * x1: 0 clean & invalidate, 1 invalidate only
 * x2~x9: clobbered
 */
.section    .warm_crt0.text.__asm_dcache_level, "ax", %progbits
.type       __asm_dcache_level, %function
__asm_dcache_level:
    lsl x12, x0, #1
    msr csselr_el1, x12     /* select cache level */
    isb                     /* sync change of cssidr_el1 */
    mrs x6, ccsidr_el1      /* read the new cssidr_el1 */
    and x2, x6, #7          /* x2 <- log2(cache line size)-4 */
    add x2, x2, #4          /* x2 <- log2(cache line size) */
    mov x3, #0x3ff
    and x3, x3, x6, lsr #3  /* x3 <- max number of #ways */
    clz w5, w3              /* bit position of #ways */
    mov x4, #0x7fff
    and x4, x4, x6, lsr #13 /* x4 <- max number of #sets */
    /* x12 <- cache level << 1 */
    /* x2 <- line length offset */
    /* x3 <- number of cache ways - 1 */
    /* x4 <- number of cache sets - 1 */
    /* x5 <- bit position of #ways */

loop_set:
    mov    x6, x3          /* x6 <- working copy of #ways */
loop_way:
    lsl x7, x6, x5
    orr x9, x12, x7     /* map way and level to cisw value */
    lsl x7, x4, x2
    orr x9, x9, x7      /* map set number to cisw value */
    tbz w1, #0, 1f
    dc isw, x9
    b 2f
1:  dc   cisw, x9        /* clean & invalidate by set/way */
2:  subs x6, x6, #1      /* decrement the way */
    b.ge loop_way
    subs x4, x4, #1     /* decrement the set */
    b.ge loop_set

    ret

/*
 * void __asm_flush_dcache_all(int invalidate_only)
 *
 * x0: 0 clean & invalidate, 1 invalidate only
 *
 * flush or invalidate all data cache by SET/WAY.
 */
.section    .warm_crt0.text.__asm_dcache_all, "ax", %progbits
.type       __asm_dcache_all, %function
__asm_dcache_all:
    mov x1, x0
    dsb sy
    mrs x10, clidr_el1      /* read clidr_el1 */
    lsr x11, x10, #24
    and x11, x11, #0x7      /* x11 <- loc */
    cbz x11, finished       /* if loc is 0, exit */
    mov x15, lr
    mov x0, #0              /* start flush at cache level 0 */
    /* x0  <- cache level */
    /* x10 <- clidr_el1 */
    /* x11 <- loc */
    /* x15 <- return address */

loop_level:
    lsl x12, x0, #1
    add x12, x12, x0        /* x0 <- tripled cache level */
    lsr x12, x10, x12
    and x12, x12, #7        /* x12 <- cache type */
    cmp x12, #2
    b.lt skip               /* skip if no cache or icache */
    bl  __asm_dcache_level  /* x1 = 0 flush, 1 invalidate */
skip:
    add x0, x0, #1          /* increment cache level */
    cmp x11, x0
    b.gt    loop_level

    mov x0, #0
    msr csselr_el1, x0      /* restore csselr_el1 */
    dsb sy
    isb
    mov lr, x15

finished:
    ret

.section    .warm_crt0.text.flush_dcache_all, "ax", %progbits
.type       flush_dcache_all, %function
.global     flush_dcache_all
flush_dcache_all:
    mov x0, #0
    b   __asm_dcache_all

.section    .warm_crt0.text.invalidate_dcache_all, "ax", %progbits
.type       invalidate_dcache_all, %function
.global     invalidate_dcache_all
invalidate_dcache_all:
    mov x0, #1
    b   __asm_dcache_all

/*
 * void __asm_flush_dcache_range(start, end) (renamed -> flush_dcache_range)
 *
 * clean & invalidate data cache in the range
 *
 * x0: start address
 * x1: end address
 */
.section    .warm_crt0.text.flush_dcache_range, "ax", %progbits
.type       flush_dcache_range, %function
.global     flush_dcache_range
flush_dcache_range:
    mrs x3, ctr_el0
    lsr x3, x3, #16
    and x3, x3, #0xf
    mov x2, #4
    lsl x2, x2, x3  /* cache line size */

    /* x2 <- minimal cache line size in cache system */
    sub x3, x2, #1
    bic x0, x0, x3
1:  dc  civac, x0   /* clean & invalidate data or unified cache */
    add x0, x0, x2
    cmp x0, x1
    b.lo    1b
    dsb sy
    ret

/*
 * void __asm_invalidate_dcache_range(start, end) (-> invalidate_dcache_range)
 *
 * invalidate data cache in the range
 *
 * x0: start address
 * x1: end address
 */
.section    .warm_crt0.text.invalidate_dcache_range, "ax", %progbits
.type       invalidate_dcache_range, %function
.global     invalidate_dcache_range
invalidate_dcache_range:
    mrs  x3, ctr_el0
    ubfm x3, x3, #16, #19
    mov x2, #4
    lsl x2, x2, x3 /* cache line size */

    /* x2 <- minimal cache line size in cache system */
    sub x3, x2, #1
    bic x0, x0, x3
1:  dc  ivac, x0    /* invalidate data or unified cache */
    add x0, x0, x2
    cmp x0, x1
    b.lo    1b
    dsb sy
    ret

/*
 * void __asm_invalidate_icache_all(void) (-> invalidate_icache_inner_shareable)
 *
 * invalidate all icache entries.
 */
.section    .warm_crt0.text.invalidate_icache_all_inner_shareable, "ax", %progbits
.type       invalidate_icache_all_inner_shareable, %function
.global     invalidate_icache_all_inner_shareable
invalidate_icache_all_inner_shareable:
    dsb ish
    isb
    ic  ialluis
    dsb ish
    isb
    ret

.section    .warm_crt0.text.invalidate_icache_all, "ax", %progbits
.type       invalidate_icache_all, %function
.global     invalidate_icache_all
invalidate_icache_all:
    dsb ish
    isb
    ic  iallu
    dsb ish
    isb
    ret

/* Final steps before power down. */
.section    .text.finalize_powerdown, "ax", %progbits
.type       finalize_powerdown, %function
.global     finalize_powerdown
finalize_powerdown:
    /*
        Make all data access to Normal memory from EL0/EL1 + all Normal Memory accesses to EL0/1 stage 1 translation tables
        non-cacheable for all levels, unified cache.
    */
    mrs x0, sctlr_el1
    bic x0, x0, #(1 << 2)
    msr sctlr_el1, x0
    isb

    /* Same as above, for EL3. */
    mrs x0, sctlr_el3
    bic x0, x0, #(1 << 2)
    msr sctlr_el3, x0
    isb

    /* Disable table walk descriptor access prefetch, disable instruction prefetch, disable data prefetch. */
    mrs x0, cpuectlr_el1
    orr x0, x0, #(1 << 38)
    bic x0, x0, #(3 << 35)
    bic x0, x0, #(3 << 32)
    msr cpuectlr_el1, x0
    isb
    dsb sy
    bl flush_dcache_all

    /* Disable receiving instruction cache/tbl maintenance operations. */
    mrs x0, cpuectlr_el1
    bic x0, x0, #(1 << 6)
    msr cpuectlr_el1, x0

    /* Prepare GICC */
    bl intr_prepare_gicc_for_sleep

    /* Set OS double lock */
    mrs x0, osdlr_el1
    orr x0, x0, #1
    msr osdlr_el1, x0

    isb
    dsb sy
wait_for_power_off:
    wfi
    b wait_for_power_off

/* Call a function with desired stack pointer. */
.section    .text.call_with_stack_pointer, "ax", %progbits
.type       call_with_stack_pointer, %function
.global     call_with_stack_pointer
call_with_stack_pointer:
    mov sp, x0
    br x1
