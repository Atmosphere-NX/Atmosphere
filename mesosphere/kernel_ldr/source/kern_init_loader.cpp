/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::kern::init::loader {

    namespace {

        constexpr size_t KernelResourceRegionSize = 0x1728000;
        constexpr size_t ExtraKernelResourceSize  = 0x68000;
        static_assert(ExtraKernelResourceSize + KernelResourceRegionSize == 0x1790000);

        constexpr size_t InitialPageTableRegionSize = 0x200000;

        class KInitialPageAllocator : public KInitialPageTable::IPageAllocator {
            private:
                uintptr_t next_address;
            public:
                constexpr ALWAYS_INLINE KInitialPageAllocator() : next_address(Null<uintptr_t>) { /* ... */ }

                ALWAYS_INLINE void Initialize(uintptr_t address) {
                    this->next_address = address;
                }

                ALWAYS_INLINE void Finalize() {
                    this->next_address = Null<uintptr_t>;
                }
            public:
                virtual KPhysicalAddress Allocate() override {
                    MESOSPHERE_ABORT_UNLESS(this->next_address != Null<uintptr_t>);
                    const uintptr_t allocated = this->next_address;
                    this->next_address += PageSize;
                    std::memset(reinterpret_cast<void *>(allocated), 0, PageSize);
                    return allocated;
                }

                /* No need to override free. The default does nothing, and so would we. */
        };

        /* Global Allocator. */
        KInitialPageAllocator g_initial_page_allocator;

        void RelocateKernelPhysically(uintptr_t &base_address, KernelLayout *&layout) {
            /* TODO: Proper secure monitor call. */
            KPhysicalAddress correct_base = KSystemControl::GetKernelPhysicalBaseAddress(base_address);
            if (correct_base != base_address) {
                const uintptr_t diff = GetInteger(correct_base) - base_address;
                const size_t size = layout->rw_end_offset;

                /* Conversion from KPhysicalAddress to void * is safe here, because MMU is not set up yet. */
                std::memmove(reinterpret_cast<void *>(GetInteger(correct_base)), reinterpret_cast<void *>(base_address), size);
                base_address += diff;
                layout = reinterpret_cast<KernelLayout *>(reinterpret_cast<uintptr_t>(layout) + diff);
            }
        }

        void SetupInitialIdentityMapping(KInitialPageTable &ttbr1_table, uintptr_t base_address, uintptr_t kernel_size, uintptr_t page_table_region, size_t page_table_region_size, KInitialPageTable::IPageAllocator &allocator) {
            /* Make a new page table for TTBR0_EL1. */
            KInitialPageTable ttbr0_table(allocator.Allocate());

            /* Map in an RWX identity mapping for the kernel. */
            constexpr PageTableEntry KernelRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable);
            ttbr0_table.Map(base_address, kernel_size, base_address, KernelRWXIdentityAttribute, allocator);

            /* Map in an RWX identity mapping for ourselves. */
            constexpr PageTableEntry KernelLdrRWXIdentityAttribute(PageTableEntry::Permission_KernelRWX, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable);
            //ttbr0_table.Map(base_address, kernel_size, base_address, KernelRWXIdentityAttribute, allocator);
        }

    }


    uintptr_t Main(uintptr_t base_address, KernelLayout *layout, uintptr_t ini_base_address) {
        /* Relocate the kernel to the correct physical base address. */
        /* Base address and layout are passed by reference and modified. */
        RelocateKernelPhysically(base_address, layout);

        /* Validate kernel layout. */
        /* TODO: constexpr 0x1000 definition somewhere. */
        /* In stratosphere, this is os::MemoryPageSize. */
        /* We don't have ams::os, this may go in hw:: or something. */
        const uintptr_t rx_offset      = layout->rx_offset;
        const uintptr_t rx_end_offset  = layout->rx_end_offset;
        const uintptr_t ro_offset      = layout->rx_offset;
        const uintptr_t ro_end_offset  = layout->ro_end_offset;
        const uintptr_t rw_offset      = layout->rx_offset;
        const uintptr_t rw_end_offset  = layout->rw_end_offset;
        const uintptr_t bss_end_offset = layout->bss_end_offset;
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(rx_offset,      0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(rx_end_offset,  0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(ro_offset,      0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(ro_end_offset,  0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(rw_offset,      0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(rw_end_offset,  0x1000));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(bss_end_offset, 0x1000));
        const uintptr_t bss_offset            = layout->bss_offset;
        const uintptr_t ini_end_offset        = layout->ini_end_offset;
        const uintptr_t dynamic_end_offset    = layout->dynamic_end_offset;
        const uintptr_t init_array_offset     = layout->init_array_offset;
        const uintptr_t init_array_end_offset = layout->init_array_end_offset;

        /* Decide if Kernel should have enlarged resource region. */
        const bool use_extra_resources = KSystemControl::ShouldIncreaseResourceRegionSize();
        const size_t resource_region_size = KernelResourceRegionSize + (use_extra_resources ? ExtraKernelResourceSize : 0);

        /* Setup the INI1 header in memory for the kernel. */
        const uintptr_t ini_end_address  = base_address + ini_end_offset + resource_region_size;
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
        SetupInitialIdentityMapping(ttbr1_table, base_address, bss_end_offset, ini_end_address, InitialPageTableRegionSize, g_initial_page_allocator);

        /* TODO: Use these. */
        (void)(bss_offset);
        (void)(ini_end_offset);
        (void)(dynamic_end_offset);
        (void)(init_array_offset);
        (void)(init_array_end_offset);


        /* TODO */
        return 0;
    }

    void Finalize() {
        g_initial_page_allocator.Finalize();
    }

}