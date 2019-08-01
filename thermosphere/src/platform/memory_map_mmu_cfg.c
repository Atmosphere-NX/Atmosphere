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
#include "../arm.h"
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
        - Shareability attribute for memory associated with translation table walks using TTBR0_EL3: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - T0SZ = from configureMemoryMap
    */
    u64 tcr = TCR_EL2_RSVD | TCR_PS(ps) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(64 - addrSpaceSize);


    /*
        - Attribute 0: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
        - Attribute 1: Device-nGnRE memory
        - Other attributes: Device-nGnRnE memory
    */
    u64 mair = 0x4FFull;

    flush_dcache_all();
    invalidate_icache_all();

    set_memory_registers_enable_mmu(ttbr0, tcr, mair);
}