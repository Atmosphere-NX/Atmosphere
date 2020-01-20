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

#include "memory_map.h"
#include "mmu.h"
#include "sysreg.h"
#include "platform/interrupt_config.h"

#define ATTRIB_MEMTYPE_NORMAL MMU_PTE_BLOCK_MEMTYPE(MEMORY_MAP_MEMTYPE_NORMAL)
#define ATTRIB_MEMTYPE_DEVICE MMU_PTE_BLOCK_MEMTYPE(MEMORY_MAP_MEMTYPE_DEVICE_NGNRE)

static uintptr_t g_currentPlatformMmioPage = MEMORY_MAP_VA_MMIO_PLAT_BASE;

void memoryMapSetupMmu(const LoadImageLayout *layout, u64 *mmuTable)
{
    static const u64 normalAttribs   =                    MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL;
    static const u64 deviceAttribs   = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_DEVICE;

    // mmuTable is currently a PA
    mmu_init_table(mmuTable, 0x200);

    /*
        Map the table into itself at the entry which index has all bits set.
        This is called "recursive page tables" and means (assuming 39-bit addr space) that:
            - the table will reuse itself as L2 table for the 0x7FC0000000+ range
            - the table will reuse itself as L3 table for the 0x7FFFE00000+ range
            - the table itself will be accessible at 0x7FFFFFF000
    */
    mmuTable[0x1FF] = (uintptr_t)mmuTable | MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_AF | MMU_PTE_TYPE_TABLE;

    /*
        Layout in physmem:
        Location1
            Image (code and data incl. BSS)
            Part of "temp" (tempbss, stacks) if there's enough space left
        Location2
            Remaining of "temp" (note: we don't and can't check if there's enough mem left!)
            MMU table (taken from temp physmem)

        Layout in vmem:
        Location1
            Image
            tempbss
        Location2
            Crash stacks
            {guard page, stack} * numCores
        Location3 (all L1, L2, L3 bits set):
            MMU table
    */

    // Map our code & data (.text/other code, .rodata, .data, .bss) at the bottom of our L3 range, all RWX
    // Note that BSS is page-aligned
    // See LD script for more details
    uintptr_t curVa = MEMORY_MAP_VA_IMAGE;
    uintptr_t curPa = layout->startPa;

    size_t tempInImageRegionMaxSize = layout->maxImageSize - layout->imageSize;
    size_t tempInImageRegionSize;
    size_t tempExtraSize;
    if (layout->tempSize <= tempInImageRegionMaxSize) {
        tempInImageRegionSize = layout->tempSize;
        tempExtraSize = 0;
    } else {
        // We need extra data
        tempInImageRegionSize = tempInImageRegionMaxSize;
        tempExtraSize = layout->tempSize - tempInImageRegionSize;
    }
    size_t imageRegionMapSize = (layout->imageSize + tempInImageRegionSize + 0xFFF) & ~0xFFFul;
    size_t tempExtraMapSize = (tempExtraSize + 0xFFF) & ~0xFFFul;

    // Do not map the MMU table in that mapping:
    mmu_map_page_range(mmuTable, curVa, curPa, imageRegionMapSize, normalAttribs);

    curVa += imageRegionMapSize;
    curPa = layout->tempPa;
    mmu_map_page_range(mmuTable, curVa, curPa, tempExtraMapSize, normalAttribs);
    curPa += tempExtraMapSize;

    // Map the remaining temporary data as stacks, aligned 0x1000

    // Crash stacks, total size is fixed:
    curVa = MEMORY_MAP_VA_CRASH_STACKS_BOTTOM;
    mmu_map_page_range(mmuTable, curVa, curPa, MEMORY_MAP_VA_CRASH_STACKS_SIZE, normalAttribs);
    curPa += MEMORY_MAP_VA_CRASH_STACKS_SIZE;

    // Regular stacks
    size_t sizePerStack = 0x1000;
    curVa = MEMORY_MAP_VA_STACKS_TOP - sizePerStack;
    for (u32 i = 0; i < 4; i++) {
        mmu_map_page_range(mmuTable, curVa, curPa, sizePerStack, normalAttribs);
        curVa -= 2 * sizePerStack;
        curPa += sizePerStack;
    }

    // MMIO
    mmu_map_page(mmuTable, MEMORY_MAP_VA_GICD, MEMORY_MAP_PA_GICD, deviceAttribs);
    mmu_map_page_range(mmuTable, MEMORY_MAP_VA_GICC, MEMORY_MAP_PA_GICC, 0x2000, deviceAttribs);
    mmu_map_page(mmuTable, MEMORY_MAP_VA_GICH, MEMORY_MAP_PA_GICH, deviceAttribs);
}

void memoryMapEnableMmu(const LoadImageLayout *layout)
{
    uintptr_t mmuTable = layout->tempPa + layout->maxTempSize;

    u32 ps = GET_SYSREG(id_aa64mmfr0_el1) & 0xF;
    /*
        - PA size: from ID_AA64MMFR0_EL1
        - Granule size: 4KB
        - Shareability attribute for memory associated with translation table walks using TTBR0_EL2: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL2: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL2: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - T0SZ = MEMORY_MAP_VA_SPACE_SIZE = 39
    */
    u64 tcr = TCR_EL2_RSVD | TCR_PS(ps) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(MEMORY_MAP_VA_SPACE_SIZE);


    /*
        - Attribute 0: Device-nGnRnE memory
        - Attribute 1: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
        - Attribute 2: Device-nGnRE memory
        - Attribute 3: Normal memory, Inner and Outer Noncacheable
        - Other attributes: Device-nGnRnE memory
    */
    u64 mair = 0x44FF0400;

    // Set VBAR because we *will* crash (instruction abort because of the value of pc) when enabling the MMU
    SET_SYSREG(vbar_el2, layout->vbar);

    // MMU regs config
    SET_SYSREG(ttbr0_el2, mmuTable);
    SET_SYSREG(tcr_el2, tcr);
    SET_SYSREG(mair_el2, mair);
    __dsb_local();
    __isb();

    // TLB invalidation
    // Whether this does anything before MMU is enabled is impldef, apparently
    __tlb_invalidate_el2_local();
    __dsb_local();
    __isb();

    // Enable MMU & enable caching. We will crash.
    u64 sctlr = GET_SYSREG(sctlr_el2);
    sctlr |= SCTLR_ELx_I | SCTLR_ELx_C | SCTLR_ELx_M;
    SET_SYSREG(sctlr_el2, sctlr);
    __dsb_local();
    __isb();
}

uintptr_t memoryMapGetStackTop(u32 coreId)
{
    return MEMORY_MAP_VA_STACKS_TOP - 0x2000 * coreId;
}

uintptr_t memoryMapPlatformMmio(uintptr_t pa, size_t size)
{
    uintptr_t va = g_currentPlatformMmioPage;
    static const u64 deviceAttribs = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_DEVICE;
    u64 *mmuTable = (u64 *)MEMORY_MAP_VA_TTBL;

    size = (size + 0xFFF) & ~0xFFFul;
    mmu_map_page_range(mmuTable, va, pa, size, deviceAttribs);

    g_currentPlatformMmioPage += size;

    return va;
}
