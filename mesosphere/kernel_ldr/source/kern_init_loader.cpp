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
#include <mesosphere.hpp>
#include "kern_init_loader_board_setup.hpp"

/* Necessary for calculating kernelldr size/base for initial identity mapping */
extern "C" {

	extern const u8 __bin_start__[];
    extern const u8 __bin_end__[];

}

namespace ams::kern::init::loader {

    namespace {

        constexpr uintptr_t KernelBaseAlignment  = 0x200000;
        constexpr uintptr_t KernelBaseRangeStart = 0xFFFFFF8000000000;
        constexpr uintptr_t KernelBaseRangeEnd   = 0xFFFFFFFFFFE00000;
        constexpr uintptr_t KernelBaseRangeLast  = KernelBaseRangeEnd - 1;
        static_assert(util::IsAligned(KernelBaseRangeStart, KernelBaseAlignment));
        static_assert(util::IsAligned(KernelBaseRangeEnd, KernelBaseAlignment));
        static_assert(KernelBaseRangeStart <= KernelBaseRangeLast);

        static_assert(InitialProcessBinarySizeMax <= KernelResourceSize);

        constexpr size_t InitialPageTableRegionSizeMax = 2_MB;
        static_assert(InitialPageTableRegionSizeMax < KernelPageTableHeapSize + KernelInitialPageHeapSize);

        /* Global Allocator. */
        constinit KInitialPageAllocator g_initial_page_allocator;

        constinit KInitialPageAllocator::State g_final_page_allocator_state;
        constinit InitialProcessBinaryLayoutWithSize g_initial_process_binary_meta;

        constinit void *g_final_state[2];

        void RelocateKernelPhysically(uintptr_t &base_address, KernelLayout *&layout, const uintptr_t &ini_base_address) {
            /* Adjust layout to be correct. */
            {
                const ptrdiff_t layout_offset = reinterpret_cast<uintptr_t>(layout) - base_address;
                layout->rx_offset             += layout_offset;
                layout->rx_end_offset         += layout_offset;
                layout->ro_offset             += layout_offset;
                layout->ro_end_offset         += layout_offset;
                layout->rw_offset             += layout_offset;
                layout->rw_end_offset         += layout_offset;
                layout->bss_offset            += layout_offset;
                layout->bss_end_offset        += layout_offset;
                layout->resource_offset       += layout_offset;
                layout->dynamic_offset        += layout_offset;
                layout->init_array_offset     += layout_offset;
                layout->init_array_end_offset += layout_offset;
                layout->sysreg_offset         += layout_offset;
            }

            /* Relocate the kernel if necessary. */
            KPhysicalAddress correct_base = KSystemControl::Init::GetKernelPhysicalBaseAddress(base_address);
            if (correct_base != base_address) {
                const uintptr_t diff = GetInteger(correct_base) - base_address;
                const size_t size = layout->rw_end_offset;

                /* Check that the new kernel doesn't overlap with us. */
                MESOSPHERE_INIT_ABORT_UNLESS((GetInteger(correct_base) >= reinterpret_cast<uintptr_t>(__bin_end__)) || (GetInteger(correct_base) + size <= reinterpret_cast<uintptr_t>(__bin_start__)));

                /* Check that the new kernel doesn't overlap with the initial process binary. */
                MESOSPHERE_INIT_ABORT_UNLESS((ini_base_address + InitialProcessBinarySizeMax <= GetInteger(correct_base)) || (GetInteger(correct_base) + size <= ini_base_address));

                /* Conversion from KPhysicalAddress to void * is safe here, because MMU is not set up yet. */
                std::memmove(reinterpret_cast<void *>(GetInteger(correct_base)), reinterpret_cast<void *>(base_address), size);
                base_address += diff;
                layout = reinterpret_cast<KernelLayout *>(reinterpret_cast<uintptr_t>(layout) + diff);
            }
        }

        void SetupInitialIdentityMapping(KInitialPageTable &init_pt, uintptr_t base_address, uintptr_t kernel_size, uintptr_t page_table_region, size_t page_table_region_size, KInitialPageAllocator &allocator, KernelSystemRegisters *sysregs) {
            /* Map in an RWX identity mapping for the kernel. */
            constexpr PageTableEntry KernelRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            init_pt.Map(base_address, kernel_size, base_address, KernelRWXIdentityAttribute, allocator, 0);

            /* Map in an RWX identity mapping for ourselves. */
            constexpr PageTableEntry KernelLdrRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            const uintptr_t kernel_ldr_base = util::AlignDown(reinterpret_cast<uintptr_t>(__bin_start__), PageSize);
            const uintptr_t kernel_ldr_size = util::AlignUp(reinterpret_cast<uintptr_t>(__bin_end__), PageSize) - kernel_ldr_base;
            init_pt.Map(kernel_ldr_base, kernel_ldr_size, kernel_ldr_base, KernelLdrRWXIdentityAttribute, allocator, 0);

            /* Map in the page table region as RW- for ourselves. */
            constexpr PageTableEntry PageTableRegionRWAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            init_pt.Map(page_table_region, page_table_region_size, page_table_region, PageTableRegionRWAttribute, allocator, 0);

            /* Place the L1 table addresses in the relevant system registers. */
            cpu::SetTtbr0El1(init_pt.GetTtbr0L1TableAddress());
            cpu::SetTtbr1El1(init_pt.GetTtbr1L1TableAddress());

            /* Setup MAIR_EL1, TCR_EL1. */
            /* TODO: Define these bits properly elsewhere, document exactly what each bit set is doing .*/
            constexpr u64 MairValue = 0x0000000044FF0400ul;
            constexpr u64 TcrValue  = 0x00000011B5193519ul;
            cpu::MemoryAccessIndirectionRegisterAccessor(MairValue).Store();
            cpu::TranslationControlRegisterAccessor(TcrValue).Store();

            /* Ensure that our configuration takes before proceeding. */
            cpu::EnsureInstructionConsistency();

            /* Perform board-specific setup. */
            PerformBoardSpecificSetup();

            /* Setup SCTLR_EL1. */
            /* TODO: Define these bits properly elsewhere, document exactly what each bit set is doing .*/
            constexpr u64 SctlrValue = 0x0000000034D5D92Dul;
            cpu::SetSctlrEl1(SctlrValue);
            cpu::InstructionMemoryBarrier();

            /* Setup the system registers for other cores. */
            /* NOTE: sctlr_el1 on other cores has the WXN bit set (0x80000); this will be set before KernelMain() on this core. */
            sysregs->ttbr0_el1 = init_pt.GetTtbr0L1TableAddress();
            sysregs->ttbr1_el1 = init_pt.GetTtbr1L1TableAddress();
            sysregs->tcr_el1   = TcrValue;
            sysregs->mair_el1  = MairValue;
            sysregs->sctlr_el1 = SctlrValue | 0x80000;
        }

        KVirtualAddress GetRandomKernelBaseAddress(KInitialPageTable &page_table, KPhysicalAddress phys_base_address, size_t kernel_size) {
            /* Define useful values for random generation. */

            const uintptr_t kernel_offset = GetInteger(phys_base_address) % KernelBaseAlignment;

            /* Repeatedly generate a random virtual address until we get one that's unmapped in the destination page table. */
            while (true) {
                const uintptr_t       random_kaslr_slide  = KSystemControl::Init::GenerateRandomRange(KernelBaseRangeStart / KernelBaseAlignment, KernelBaseRangeLast / KernelBaseAlignment);
                const KVirtualAddress kernel_region_start = random_kaslr_slide * KernelBaseAlignment;
                const KVirtualAddress kernel_region_end   = kernel_region_start + util::AlignUp(kernel_offset + kernel_size, KernelBaseAlignment);
                const size_t          kernel_region_size  = GetInteger(kernel_region_end) - GetInteger(kernel_region_start);

                /* Make sure the region has not overflowed */
                if (kernel_region_start >= kernel_region_end) {
                    continue;
                }

                /* Make sure that the region stays within our intended bounds. */
                if (kernel_region_end > KernelBaseRangeEnd) {
                    continue;
                }

                /* Validate we can map the range we've selected. */
                if (!page_table.IsFree(kernel_region_start, kernel_region_size)) {
                    continue;
                }

                /* Our range is valid! */
                return kernel_region_start + kernel_offset;
            }
        }

    }

    uintptr_t Main(uintptr_t base_address, KernelLayout *layout, uintptr_t ini_base_address) {
        /* Relocate the kernel to the correct physical base address. */
        /* Base address and layout are passed by reference and modified. */
        RelocateKernelPhysically(base_address, layout, ini_base_address);

        /* Validate kernel layout. */
        const uintptr_t rx_offset      = layout->rx_offset;
        const uintptr_t rx_end_offset  = layout->rx_end_offset;
        const uintptr_t ro_offset      = layout->ro_offset;
        const uintptr_t ro_end_offset  = layout->ro_end_offset;
        const uintptr_t rw_offset      = layout->rw_offset;
        /* UNUSED: const uintptr_t rw_end_offset  = layout->rw_end_offset; */
        const uintptr_t bss_end_offset = layout->bss_end_offset;
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(rx_offset,      PageSize));
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(rx_end_offset,  PageSize));
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(ro_offset,      PageSize));
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(ro_end_offset,  PageSize));
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(rw_offset,      PageSize));
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(bss_end_offset, PageSize));
        const uintptr_t bss_offset            = layout->bss_offset;
        const uintptr_t resource_offset       = layout->resource_offset;
        const uintptr_t dynamic_offset        = layout->dynamic_offset;
        const uintptr_t init_array_offset     = layout->init_array_offset;
        const uintptr_t init_array_end_offset = layout->init_array_end_offset;
        const uintptr_t sysreg_offset         = layout->sysreg_offset;

        /* Determine the size of the resource region. */
        const size_t resource_region_size = KMemoryLayout::GetResourceRegionSizeForInit(KSystemControl::Init::ShouldIncreaseThreadResourceLimit());
        const uintptr_t resource_end_address  = base_address + resource_offset + resource_region_size;

        /* Setup the INI1 header in memory for the kernel. */
        {
            /* Get the kernel layout. */
            KSystemControl::Init::GetInitialProcessBinaryLayout(std::addressof(g_initial_process_binary_meta.layout), base_address);

            /* If there's no desired base address, use the ini in place. */
            if (g_initial_process_binary_meta.layout.address == 0) {
                g_initial_process_binary_meta.layout.address = ini_base_address;
            }


            /* Validate and potentially relocate the INI. */
            const InitialProcessBinaryHeader *ini_header = reinterpret_cast<const InitialProcessBinaryHeader *>(ini_base_address);
            size_t ini_size = 0;
            if (ini_header->magic == InitialProcessBinaryMagic && (ini_size = ini_header->size) <= InitialProcessBinarySizeMax) {
                /* INI is valid, relocate it if necessary. */
                if (ini_base_address != g_initial_process_binary_meta.layout.address) {
                    std::memmove(reinterpret_cast<void *>(g_initial_process_binary_meta.layout.address), ini_header, ini_size);
                }
            } else {
                /* INI is invalid. Make the destination header invalid. */
                std::memset(reinterpret_cast<void *>(g_initial_process_binary_meta.layout.address), 0, sizeof(InitialProcessBinaryHeader));
            }

            /* Set the INI size in layout. */
            g_initial_process_binary_meta.size = util::AlignUp(ini_size, PageSize);
        }

        /* We want to start allocating page tables at the end of the resource region. */
        g_initial_page_allocator.Initialize(resource_end_address);

        /* Make a new page table for TTBR1_EL1. */
        KInitialPageTable init_pt(KernelBaseRangeStart, KernelBaseRangeLast, g_initial_page_allocator);

        /* Setup initial identity mapping. TTBR1 table passed by reference. */
        SetupInitialIdentityMapping(init_pt, base_address, bss_end_offset, resource_end_address, InitialPageTableRegionSizeMax, g_initial_page_allocator, reinterpret_cast<KernelSystemRegisters *>(base_address + sysreg_offset));

        /* NOTE: On 19.0.0+, Nintendo calls an unknown function here on init_pt and g_initial_page_allocator. */
        /* This is stubbed in prod KernelLdr. */

        /* Generate a random slide for the kernel's base address. */
        const KVirtualAddress virtual_base_address = GetRandomKernelBaseAddress(init_pt, base_address, bss_end_offset);

        /* Map kernel .text as R-X. */
        constexpr PageTableEntry KernelTextAttribute(PageTableEntry::Permission_KernelRX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        init_pt.Map(virtual_base_address + rx_offset, rx_end_offset - rx_offset, base_address + rx_offset, KernelTextAttribute, g_initial_page_allocator, 0);

        /* Map kernel .rodata and .rwdata as RW-. */
        /* Note that we will later reprotect .rodata as R-- */
        constexpr PageTableEntry KernelRoDataAttribute(PageTableEntry::Permission_KernelR, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        init_pt.Map(virtual_base_address + ro_offset, ro_end_offset - ro_offset, base_address + ro_offset, KernelRwDataAttribute, g_initial_page_allocator, 0);
        init_pt.Map(virtual_base_address + rw_offset, bss_end_offset - rw_offset, base_address + rw_offset, KernelRwDataAttribute, g_initial_page_allocator, 0);

        /* Physically randomize the kernel region. */
        /* NOTE: Nintendo does this only on 10.0.0+ */
        init_pt.PhysicallyRandomize(virtual_base_address + rx_offset, bss_end_offset - rx_offset, true);

        /* Apply relocations to the kernel. */
        const Elf::Dyn *kernel_dynamic = reinterpret_cast<const Elf::Dyn *>(GetInteger(virtual_base_address) + dynamic_offset);
        Elf::ApplyRelocations(GetInteger(virtual_base_address), kernel_dynamic);

        /* Clear kernel .bss. */
        /* NOTE: The kernel does this before applying relocations, but we do it after. */
        /* This allows us to place our relocations in space overlapping with .bss...and thereby reclaim the memory that would otherwise be wasted. */
        std::memset(GetVoidPointer(virtual_base_address + bss_offset), 0, bss_end_offset - bss_offset);

        /* Call the kernel's init array functions. */
        /* NOTE: The kernel does this after reprotecting .rodata, but we do it before. */
        /* This allows our global constructors to edit .rodata, which is valuable for editing the SVC tables to support older firmwares' ABIs. */
        Elf::CallInitArrayFuncs(GetInteger(virtual_base_address) + init_array_offset, GetInteger(virtual_base_address) + init_array_end_offset);

        /* Reprotect .rodata as R-- */
        init_pt.Reprotect(virtual_base_address + ro_offset, ro_end_offset - ro_offset, KernelRwDataAttribute, KernelRoDataAttribute);

        /* Return the difference between the random virtual base and the physical base. */
        return GetInteger(virtual_base_address) - base_address;
    }

    KPhysicalAddress AllocateKernelInitStack() {
        return g_initial_page_allocator.Allocate(PageSize) + PageSize;
    }

    void **GetFinalState() {
        /* Get final page allocator state. */
        g_initial_page_allocator.GetFinalState(std::addressof(g_final_page_allocator_state));

        /* Setup final kernel loader state. */
        g_final_state[0] = std::addressof(g_final_page_allocator_state);
        g_final_state[1] = std::addressof(g_initial_process_binary_meta);

        return g_final_state;
    }

}