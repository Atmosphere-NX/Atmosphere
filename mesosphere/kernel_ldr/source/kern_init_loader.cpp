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
#include <mesosphere.hpp>
#include "kern_init_loader_board_setup.hpp"

/* Necessary for calculating kernelldr size/base for initial identity mapping */
extern "C" {

	extern const u8 __start__[];
    extern const u8 __end__[];

}

namespace ams::kern::init::loader {

    namespace {

        static_assert(InitialProcessBinarySizeMax <= KernelResourceSize);

        constexpr size_t InitialPageTableRegionSizeMax = 2_MB;
        static_assert(InitialPageTableRegionSizeMax < KernelPageTableHeapSize + KernelInitialPageHeapSize);

        /* Global Allocator. */
        KInitialPageAllocator g_initial_page_allocator;

        KInitialPageAllocator::State g_final_page_allocator_state;

        void RelocateKernelPhysically(uintptr_t &base_address, KernelLayout *&layout) {
            KPhysicalAddress correct_base = KSystemControl::Init::GetKernelPhysicalBaseAddress(base_address);
            if (correct_base != base_address) {
                const uintptr_t diff = GetInteger(correct_base) - base_address;
                const size_t size = layout->rw_end_offset;

                /* Conversion from KPhysicalAddress to void * is safe here, because MMU is not set up yet. */
                std::memmove(reinterpret_cast<void *>(GetInteger(correct_base)), reinterpret_cast<void *>(base_address), size);
                base_address += diff;
                layout = reinterpret_cast<KernelLayout *>(reinterpret_cast<uintptr_t>(layout) + diff);
            }
        }

        void EnsureEntireDataCacheFlushed() {
            /* Flush shared cache. */
            cpu::FlushEntireDataCacheSharedForInit();
            cpu::DataSynchronizationBarrier();

            /* Flush local cache. */
            cpu::FlushEntireDataCacheLocalForInit();
            cpu::DataSynchronizationBarrier();

            /* Flush shared cache. */
            cpu::FlushEntireDataCacheSharedForInit();
            cpu::DataSynchronizationBarrier();

            /* Invalidate entire instruction cache. */
            cpu::InvalidateEntireInstructionCacheForInit();

            /* Invalidate entire TLB. */
            cpu::InvalidateEntireTlb();
        }

        void SetupInitialIdentityMapping(KInitialPageTable &ttbr1_table, uintptr_t base_address, uintptr_t kernel_size, uintptr_t page_table_region, size_t page_table_region_size, KInitialPageTable::IPageAllocator &allocator) {
            /* Make a new page table for TTBR0_EL1. */
            KInitialPageTable ttbr0_table(allocator.Allocate());

            /* Map in an RWX identity mapping for the kernel. */
            constexpr PageTableEntry KernelRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            ttbr0_table.Map(base_address, kernel_size, base_address, KernelRWXIdentityAttribute, allocator);

            /* Map in an RWX identity mapping for ourselves. */
            constexpr PageTableEntry KernelLdrRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            const uintptr_t kernel_ldr_base = util::AlignDown(reinterpret_cast<uintptr_t>(__start__), PageSize);
            const uintptr_t kernel_ldr_size = util::AlignUp(reinterpret_cast<uintptr_t>(__end__), PageSize) - kernel_ldr_base;
            ttbr0_table.Map(kernel_ldr_base, kernel_ldr_size, kernel_ldr_base, KernelRWXIdentityAttribute, allocator);

            /* Map in the page table region as RW- for ourselves. */
            constexpr PageTableEntry PageTableRegionRWAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
            ttbr0_table.Map(page_table_region, page_table_region_size, page_table_region, KernelRWXIdentityAttribute, allocator);

            /* Place the L1 table addresses in the relevant system registers. */
            cpu::SetTtbr0El1(ttbr0_table.GetL1TableAddress());
            cpu::SetTtbr1El1(ttbr1_table.GetL1TableAddress());

            /* Setup MAIR_EL1, TCR_EL1. */
            /* TODO: Define these bits properly elsewhere, document exactly what each bit set is doing .*/
            constexpr u64 MairValue = 0x0000000044FF0400ul;
            constexpr u64 TcrValue  = 0x00000011B5193519ul;
            cpu::MemoryAccessIndirectionRegisterAccessor(MairValue).Store();
            cpu::TranslationControlRegisterAccessor(TcrValue).Store();

            /* Perform board-specific setup. */
            PerformBoardSpecificSetup();

            /* Ensure that the entire cache is flushed. */
            EnsureEntireDataCacheFlushed();

            /* Setup SCTLR_EL1. */
            /* TODO: Define these bits properly elsewhere, document exactly what each bit set is doing .*/
            constexpr u64 SctlrValue = 0x0000000034D5D925ul;
            cpu::SetSctlrEl1(SctlrValue);
            cpu::EnsureInstructionConsistency();
        }

        KVirtualAddress GetRandomKernelBaseAddress(KInitialPageTable &page_table, KPhysicalAddress phys_base_address, size_t kernel_size) {
            /* Define useful values for random generation. */
            constexpr uintptr_t KernelBaseAlignment = 0x200000;
            constexpr uintptr_t KernelBaseRangeMin  = 0xFFFFFF8000000000;
            constexpr uintptr_t KernelBaseRangeMax  = 0xFFFFFFFFFFE00000;
            constexpr uintptr_t KernelBaseRangeEnd = KernelBaseRangeMax - 1;
            static_assert(util::IsAligned(KernelBaseRangeMin, KernelBaseAlignment));
            static_assert(util::IsAligned(KernelBaseRangeMax, KernelBaseAlignment));
            static_assert(KernelBaseRangeMin <= KernelBaseRangeEnd);

            const uintptr_t kernel_offset = GetInteger(phys_base_address) % KernelBaseAlignment;

            /* Repeatedly generate a random virtual address until we get one that's unmapped in the destination page table. */
            while (true) {
                const uintptr_t       random_kaslr_slide  = KSystemControl::Init::GenerateRandomRange(KernelBaseRangeMin / KernelBaseAlignment, KernelBaseRangeEnd / KernelBaseAlignment);
                const KVirtualAddress kernel_region_start = random_kaslr_slide * KernelBaseAlignment;
                const KVirtualAddress kernel_region_end   = kernel_region_start + util::AlignUp(kernel_offset + kernel_size, KernelBaseAlignment);
                const size_t          kernel_region_size  = GetInteger(kernel_region_end) - GetInteger(kernel_region_start);

                /* Make sure the region has not overflowed */
                if (kernel_region_start >= kernel_region_end) {
                    continue;
                }

                /* Make sure that the region stays within our intended bounds. */
                if (kernel_region_end > KernelBaseRangeMax) {
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
        RelocateKernelPhysically(base_address, layout);

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
        const uintptr_t ini_load_offset       = layout->ini_load_offset;
        const uintptr_t dynamic_offset        = layout->dynamic_offset;
        const uintptr_t init_array_offset     = layout->init_array_offset;
        const uintptr_t init_array_end_offset = layout->init_array_end_offset;

        /* Determine the size of the resource region. */
        const size_t resource_region_size = KMemoryLayout::GetResourceRegionSizeForInit();

        /* Setup the INI1 header in memory for the kernel. */
        const uintptr_t ini_end_address  = base_address + ini_load_offset + resource_region_size;
        const uintptr_t ini_load_address = ini_end_address - InitialProcessBinarySizeMax;
        if (ini_base_address != ini_load_address) {
            /* The INI is not at the correct address, so we need to relocate it. */
            const InitialProcessBinaryHeader *ini_header = reinterpret_cast<const InitialProcessBinaryHeader *>(ini_base_address);
            if (ini_header->magic == InitialProcessBinaryMagic && ini_header->size <= InitialProcessBinarySizeMax) {
                /* INI is valid, relocate it. */
                std::memmove(reinterpret_cast<void *>(ini_load_address), ini_header, ini_header->size);
            } else {
                /* INI is invalid. Make the destination header invalid. */
                std::memset(reinterpret_cast<void *>(ini_load_address), 0, sizeof(InitialProcessBinaryHeader));
            }
        }

        /* We want to start allocating page tables at ini_end_address. */
        g_initial_page_allocator.Initialize(ini_end_address);

        /* Make a new page table for TTBR1_EL1. */
        KInitialPageTable ttbr1_table(g_initial_page_allocator.Allocate());

        /* Setup initial identity mapping. TTBR1 table passed by reference. */
        SetupInitialIdentityMapping(ttbr1_table, base_address, bss_end_offset, ini_end_address, InitialPageTableRegionSizeMax, g_initial_page_allocator);

        /* Generate a random slide for the kernel's base address. */
        const KVirtualAddress virtual_base_address = GetRandomKernelBaseAddress(ttbr1_table, base_address, bss_end_offset);

        /* Map kernel .text as R-X. */
        constexpr PageTableEntry KernelTextAttribute(PageTableEntry::Permission_KernelRX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        ttbr1_table.Map(virtual_base_address + rx_offset, rx_end_offset - rx_offset, base_address + rx_offset, KernelTextAttribute, g_initial_page_allocator);

        /* Map kernel .rodata and .rwdata as RW-. */
        /* Note that we will later reprotect .rodata as R-- */
        constexpr PageTableEntry KernelRoDataAttribute(PageTableEntry::Permission_KernelR, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable, PageTableEntry::MappingFlag_Mapped);
        ttbr1_table.Map(virtual_base_address + ro_offset, ro_end_offset - ro_offset, base_address + ro_offset, KernelRwDataAttribute, g_initial_page_allocator);
        ttbr1_table.Map(virtual_base_address + rw_offset, bss_end_offset - rw_offset, base_address + rw_offset, KernelRwDataAttribute, g_initial_page_allocator);

        /* Physically randomize the kernel region. */
        /* NOTE: Nintendo does this only on 10.0.0+ */
        ttbr1_table.PhysicallyRandomize(virtual_base_address + rx_offset, bss_end_offset - rx_offset, true);

        /* Clear kernel .bss. */
        std::memset(GetVoidPointer(virtual_base_address + bss_offset), 0, bss_end_offset - bss_offset);

        /* Apply relocations to the kernel. */
        const Elf::Dyn *kernel_dynamic = reinterpret_cast<const Elf::Dyn *>(GetInteger(virtual_base_address) + dynamic_offset);
        Elf::ApplyRelocations(GetInteger(virtual_base_address), kernel_dynamic);

        /* Call the kernel's init array functions. */
        /* NOTE: The kernel does this after reprotecting .rodata, but we do it before. */
        /* This allows our global constructors to edit .rodata, which is valuable for editing the SVC tables to support older firmwares' ABIs. */
        Elf::CallInitArrayFuncs(GetInteger(virtual_base_address) + init_array_offset, GetInteger(virtual_base_address) + init_array_end_offset);

        /* Reprotect .rodata as R-- */
        ttbr1_table.Reprotect(virtual_base_address + ro_offset, ro_end_offset - ro_offset, KernelRwDataAttribute, KernelRoDataAttribute);

        /* Return the difference between the random virtual base and the physical base. */
        return GetInteger(virtual_base_address) - base_address;
    }

    KPhysicalAddress AllocateKernelInitStack() {
        return g_initial_page_allocator.Allocate() + PageSize;
    }

    uintptr_t GetFinalPageAllocatorState() {
        g_initial_page_allocator.GetFinalState(std::addressof(g_final_page_allocator_state));
        return reinterpret_cast<uintptr_t>(std::addressof(g_final_page_allocator_state));
    }

}