/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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


#include "hvisor_memory_map.hpp"
#include "hvisor_core_context.hpp"

#include "cpu/hvisor_cpu_mmu.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

namespace ams::hvisor {

    uintptr_t MemoryMap::currentPlatformMmioPage = MemoryMap::mmioPlatBaseVa;

    void MemoryMap::SetupMmu(const MemoryMap::LoadImageLayout *layout)
    {
        using namespace cpu;

        constexpr u64 normalAttribs =          MMU_INNER_SHAREABLE | MMU_ATTRINDX(Memtype_Normal);
        constexpr u64 deviceAttribs = MMU_XN | MMU_INNER_SHAREABLE | MMU_ATTRINDX(Memtype_Device_nGnRE);

        /*
            Layout in physmem:
            Location1
                Image (code and data incl. BSS), which size is page-aligned
            Location2
                tempbss
                MMU table (taken from temp physmem)

            Layout in vmem:
            Location1
                Image
                padding
                tempbss
            Location2
                Crash stacks
                {guard page, stack} * numCores
            Location3 (all L1, L2, L3 bits set):
                MMU table

            We map the table into itself at the entry which index has all bits set.
            This is called "recursive page tables" and means (assuming 39-bit addr space) that:
                - the table will reuse itself as L2 table for the 0x7FC0000000+ range
                - the table will reuse itself as L3 table for the 0x7FFFE00000+ range
                - the table itself will be accessible at 0x7FFFFFF000
         */

        using Builder = MmuTableBuilder<3, addressSpaceSize>;
        uintptr_t mmuTablePa = layout->tempPa + layout->maxTempSize;

        uintptr_t tempVa = imageVa + layout->imageSize;
        uintptr_t crashStacksPa = layout->tempPa + layout->tempSize;
        uintptr_t stacksPa = crashStacksPa + crashStacksSize;

        Builder{reinterpret_cast<u64 *>(mmuTablePa)}
            .InitializeTable()
            // Image & tempbss & crash stacks
            .MapBlockRange(imageVa, layout->startPa, layout->imageSize, normalAttribs)
            .MapBlockRange(tempVa, layout->tempPa, layout->tempSize, normalAttribs)
            .MapBlockRange(crashStacksBottomVa, crashStacksPa, crashStacksSize, normalAttribs)
            // Stacks, each with a guard page
            .MapBlockRange(stacksBottomVa, stacksPa, 0x1000ul * MAX_CORE, normalAttribs, 0x1000)
            // GICD, GICC, GICH
            .MapBlock(gicdVa, MEMORY_MAP_PA_GICD, deviceAttribs)
            .MapBlockRange(giccVa, MEMORY_MAP_PA_GICC, 0x2000, deviceAttribs)
            .MapBlock(gichVa, MEMORY_MAP_PA_GICH, deviceAttribs)
            // Recursive page mapping
            .MapBlock(ttblVa, mmuTablePa, normalAttribs)
        ;
    }

    std::array<uintptr_t, 2> MemoryMap::EnableMmuGetStacks(const MemoryMap::LoadImageLayout *layout, u32 coreId)
    {
        using namespace cpu;
        uintptr_t mmuTablePa = layout->tempPa + layout->maxTempSize;

        u32 ps = THERMOSPHERE_GET_SYSREG(id_aa64mmfr0_el1) & 0xF;
        /*
            - PA size: from ID_AA64MMFR0_EL1
            - Granule size: 4KB
            - Shareability attribute for memory associated with translation table walks using TTBR0_EL2:
                Inner Shareable
            - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL2:
                Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
            - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL2:
                Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
            - T0SZ = 39
        */
        u64 tcr = TCR_EL2_RSVD | TCR_PS(ps) | TCR_TG0(TranslationGranule_4K) | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(addressSpaceSize);


        /*
            - Attribute 0: Device-nGnRnE memory
            - Attribute 1: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
            - Attribute 2: Device-nGnRE memory
            - Attribute 3: Normal memory, Inner and Outer Noncacheable
            - Other attributes: Device-nGnRnE memory
        */
        constexpr u64 mair = 0x44FF0400;

        // Set VBAR because we *will* crash (instruction abort because of the value of pc) when enabling the MMU
        THERMOSPHERE_SET_SYSREG(vbar_el2, layout->vbar);

        // MMU regs config
        THERMOSPHERE_SET_SYSREG(ttbr0_el2, mmuTablePa);
        THERMOSPHERE_SET_SYSREG(tcr_el2, tcr);
        THERMOSPHERE_SET_SYSREG(mair_el2, mair);
        dsb();
        isb();

        // TLB invalidation
        // Whether this does anything before MMU is enabled is impldef, apparently
        TlbInvalidateEl2Local();
        dsb();
        isb();

        // Enable MMU & enable caching. We will crash.
        u64 sctlr = THERMOSPHERE_GET_SYSREG(sctlr_el2);
        sctlr |= SCTLR_ELx_I | SCTLR_ELx_C | SCTLR_ELx_M;
        THERMOSPHERE_SET_SYSREG(sctlr_el2, sctlr);
        dsb();
        isb();

        // crashStackTop is fragile, check if crashStacksSize is suitable for MAX_CORE
        uintptr_t stackTop = stacksBottomVa + 0x2000 * coreId + 0x1000;
        uintptr_t crashStackTop = crashStacksBottomVa + (crashStacksSize / MAX_CORE) * (1 + coreId);
        return std::array{stackTop, crashStackTop};
    }

    uintptr_t MemoryMap::MapPlatformMmio(uintptr_t pa, size_t size)
    {
        using namespace cpu;
        using Builder = MmuTableBuilder<3, addressSpaceSize, true>;
        constexpr u64 deviceAttribs = MMU_XN | MMU_INNER_SHAREABLE | MMU_ATTRINDX(Memtype_Device_nGnRE);

        uintptr_t va = currentPlatformMmioPage;
        size = (size + 0xFFF) & ~0xFFFul;
        Builder{reinterpret_cast<u64 *>(ttblVa)}.MapBlockRange(currentPlatformMmioPage, va, size, deviceAttribs);

        currentPlatformMmioPage += size;
        return va;
    }

    uintptr_t MemoryMap::MapGuestPage(uintptr_t pa, u64 memAttribs, u64 shareability)
    {
        using namespace cpu;
        using Builder = MmuTableBuilder<3, addressSpaceSize, true>;

        u64 attribs = MMU_XN | MMU_SH(shareability) | MMU_ATTRINDX(Memtype_Guest_Slot);
        uintptr_t va = guestMemVa + 0x2000 * currentCoreCtx->GetCoreId(); // one guard page

        // Update mair_el2
        u64 mair = THERMOSPHERE_GET_SYSREG(mair_el2);
        mair |= memAttribs << (8 * Memtype_Guest_Slot);
        THERMOSPHERE_SET_SYSREG(mair_el2, mair);
        isb();

        Builder{reinterpret_cast<u64 *>(ttblVa)}.MapBlock(va, pa, attribs);
        TlbInvalidateEl2Page(va);
        dsb();
        isb();
    }

    void MemoryMap::UnmapGuestPage()
    {
        using namespace cpu;
        using Builder = MmuTableBuilder<3, addressSpaceSize, true>;
        uintptr_t va = guestMemVa + 0x2000 * currentCoreCtx->GetCoreId();

        dsb();
        isb();

        Builder{reinterpret_cast<u64 *>(ttblVa)}.Unmap(va);
        TlbInvalidateEl2Page(va);
        dsb();
        isb();

        // Update mair_el2
        u64 mair = THERMOSPHERE_GET_SYSREG(mair_el2);
        mair &= ~(0xFF << (8 * Memtype_Guest_Slot));
        THERMOSPHERE_SET_SYSREG(mair_el2, mair);
        isb();
    }
}
