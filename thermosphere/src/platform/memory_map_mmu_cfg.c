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

#include "../utils.h"
#include "../sysreg.h"
#include "../mmu.h"
#include "memory_map_mmu_cfg.h"

void configureMemoryMapEnableMmu(void)
{
    u32 addrSpaceSize;
    uintptr_t ttbr0 = configureMemoryMap(&addrSpaceSize);

    u32 ps = GET_SYSREG(id_aa64mmfr0_el1) & 0xF;
    /*
        - PA size: from ID_AA64MMFR0_EL1
        - Granule size: 4KB
        - Shareability attribute for memory associated with translation table walks using TTBR0_EL2: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL2: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL2: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - T0SZ = from configureMemoryMap
    */
    u64 tcr = TCR_EL2_RSVD | TCR_PS(ps) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(addrSpaceSize);


    /*
        - Attribute 0: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
        - Attribute 1: Device-nGnRE memory
        - Other attributes: Device-nGnRnE memory
    */
    u64 mair = 0x4FFull;

    // MMU regs config
    SET_SYSREG(ttbr0_el2, ttbr0);
    SET_SYSREG(tcr_el2, tcr);
    SET_SYSREG(mair_el2, mair);
    __dsb();
    __isb();

    // TLB invalidation
    __tlb_invalidate_el2();
    __dsb();
    __isb();

    // Enable MMU & enable caching
    u64 sctlr = GET_SYSREG(sctlr_el2);
    sctlr |= SCTLR_ELx_I | SCTLR_ELx_C | SCTLR_ELx_M;
    SET_SYSREG(sctlr_el2, sctlr);
    __dsb();
    __isb();
}

void configureMemoryMapEnableStage2(void)
{
    u32 addrSpaceSize;
    uintptr_t vttbr = configureStage2MemoryMap(&addrSpaceSize);

    u32 ps = GET_SYSREG(id_aa64mmfr0_el1) & 0xF;
    /*
        - PA size: from ID_AA64MMFR0_EL1
        - Granule size: 4KB
        - Shareability attribute for memory associated with translation table walks using VTTBR_EL2: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using VTTBR_EL2: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using VTTBR_EL2: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - SL0 = start at level 1
        - T0SZ = from configureMemoryMap
    */
    u64 vtcr = VTCR_EL2_RSVD | TCR_PS(ps) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | VTCR_SL0(1) | TCR_T0SZ(addrSpaceSize);

    // Stage2 regs config
    SET_SYSREG(vttbr_el2, vttbr);
    SET_SYSREG(vtcr_el2, vtcr);
    __dsb();
    __isb();

    // TLB invalidation
    __tlb_invalidate_el1_stage12();
    __dsb();
    __isb();

    // Enable stage 2
    u64 hcr = GET_SYSREG(hcr_el2);
    hcr |= HCR_VM;
    SET_SYSREG(hcr_el2, hcr);
    __dsb();
    __isb();
}
