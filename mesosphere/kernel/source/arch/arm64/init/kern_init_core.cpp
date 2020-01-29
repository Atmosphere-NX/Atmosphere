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

extern "C" void _start();
extern "C" void __end__();

namespace ams::kern::init {

    /* Prototypes for functions declared in ASM that we need to reference. */
    void StartOtherCore(const ams::kern::init::KInitArguments *init_args);

    namespace {

        constexpr size_t KernelResourceRegionSize = 0x1728000;
        constexpr size_t ExtraKernelResourceSize  = 0x68000;
        static_assert(ExtraKernelResourceSize + KernelResourceRegionSize == 0x1790000);

        /* Global Allocator. */
        KInitialPageAllocator g_initial_page_allocator;

        /* Global initial arguments array. */
        KPhysicalAddress g_init_arguments_phys_addr[cpu::NumCores];

        /* Page table attributes. */
        constexpr PageTableEntry KernelRoDataAttribute(PageTableEntry::Permission_KernelR,  PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable);
        constexpr PageTableEntry KernelRwDataAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_NormalMemory, PageTableEntry::Shareable_InnerShareable);
        constexpr PageTableEntry KernelMmioAttribute(PageTableEntry::Permission_KernelRW, PageTableEntry::PageAttribute_Device_nGnRE, PageTableEntry::Shareable_OuterShareable);

        void MapStackForCore(KInitialPageTable &page_table, KMemoryRegionType type, u32 core_id) {
            constexpr size_t StackSize  = PageSize;
            constexpr size_t StackAlign = PageSize;
            const KVirtualAddress  stack_start_virt = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegionWithGuard(StackSize, StackAlign, KMemoryRegionType_KernelMisc, PageSize);
            const KPhysicalAddress stack_start_phys = g_initial_page_allocator.Allocate();
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(stack_start_virt), StackSize, type, core_id));

            page_table.Map(stack_start_virt, StackSize, stack_start_phys, KernelRwDataAttribute, g_initial_page_allocator);
        }

        void StoreDataCache(const void *addr, size_t size) {
            uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr), cpu::DataCacheLineSize);
            uintptr_t end   = reinterpret_cast<uintptr_t>(addr) + size;
            for (uintptr_t cur = start; cur < end; cur += cpu::DataCacheLineSize) {
                __asm__ __volatile__("dc cvac, %[cur]" :: [cur]"r"(cur) : "memory");
            }
            cpu::DataSynchronizationBarrier();
        }

        void TurnOnAllCores(uintptr_t start_other_core_phys) {
            cpu::MultiprocessorAffinityRegisterAccessor mpidr;
            const auto arg = mpidr.GetCpuOnArgument();
            const auto current_core = mpidr.GetAff0();

            for (s32 i = 0; i < static_cast<s32>(cpu::NumCores); i++) {
                if (static_cast<s32>(current_core) != i) {
                    KSystemControl::Init::CpuOn(arg | i, start_other_core_phys, GetInteger(g_init_arguments_phys_addr[i]));
                }
            }
        }

    }

    void InitializeCore(uintptr_t misc_unk_debug_phys_addr, uintptr_t initial_page_allocator_state) {
        /* Ensure our first argument is page aligned (as we will map it if it is non-zero). */
        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(misc_unk_debug_phys_addr, PageSize));

        /* Clear TPIDR_EL1 to zero. */
        cpu::ThreadIdRegisterAccessor(0).Store();

        /* Restore the page allocator state setup by kernel loader. */
        g_initial_page_allocator.Initialize(initial_page_allocator_state);

        /* Ensure that the T1SZ is correct (and what we expect). */
        MESOSPHERE_INIT_ABORT_UNLESS((cpu::TranslationControlRegisterAccessor().GetT1Size() / L1BlockSize) == MaxPageTableEntries);

        /* Create page table object for use during initialization. */
        KInitialPageTable ttbr1_table(util::AlignDown(cpu::GetTtbr1El1(), PageSize), KInitialPageTable::NoClear{});

        /* Initialize the slab allocator counts. */
        /* TODO */

        /* Insert the root block for the virtual memory tree, from which all other blocks will derive. */
        KMemoryLayout::GetVirtualMemoryBlockTree().insert(*KMemoryLayout::GetMemoryBlockAllocator().Create(KernelVirtualAddressSpaceBase, KernelVirtualAddressSpaceSize, 0, 0));

        /* Insert the root block for the physical memory tree, from which all other blocks will derive. */
        KMemoryLayout::GetPhysicalMemoryBlockTree().insert(*KMemoryLayout::GetMemoryBlockAllocator().Create(KernelPhysicalAddressSpaceBase, KernelPhysicalAddressSpaceSize, 0, 0));

        /* Save start and end for ease of use. */
        const uintptr_t code_start_virt_addr = reinterpret_cast<uintptr_t>(_start);
        const uintptr_t code_end_virt_addr   = reinterpret_cast<uintptr_t>(__end__);

        /* Setup the containing kernel region. */
        constexpr size_t KernelRegionSize  = 1_GB;
        constexpr size_t KernelRegionAlign = 1_GB;
        const KVirtualAddress kernel_region_start = util::AlignDown(code_start_virt_addr, KernelRegionAlign);
        size_t kernel_region_size = KernelRegionSize;
        if (!(kernel_region_start + KernelRegionSize - 1 <= KernelVirtualAddressSpaceLast)) {
            kernel_region_size = KernelVirtualAddressSpaceEnd - GetInteger(kernel_region_start);
        }
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(kernel_region_start), kernel_region_size, KMemoryRegionType_Kernel));

        /* Setup the code region. */
        constexpr size_t CodeRegionAlign = PageSize;
        const KVirtualAddress code_region_start = util::AlignDown(code_start_virt_addr, CodeRegionAlign);
        const KVirtualAddress code_region_end = util::AlignUp(code_end_virt_addr, CodeRegionAlign);
        const size_t code_region_size = GetInteger(code_region_end) - GetInteger(code_region_start);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(code_region_start), code_region_size, KMemoryRegionType_KernelCode));

        /* Setup the misc region. */
        constexpr size_t MiscRegionSize  = 32_MB;
        constexpr size_t MiscRegionAlign = KernelAslrAlignment;
        const KVirtualAddress misc_region_start = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegion(MiscRegionSize, MiscRegionAlign, KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(misc_region_start), MiscRegionSize, KMemoryRegionType_KernelMisc));

        /* Setup the stack region. */
        constexpr size_t StackRegionSize  = 14_MB;
        constexpr size_t StackRegionAlign = KernelAslrAlignment;
        const KVirtualAddress stack_region_start = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegion(StackRegionSize, StackRegionAlign, KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(stack_region_start), StackRegionSize, KMemoryRegionType_KernelStack));

        /* Decide if Kernel should have enlarged resource region (slab region + page table heap region). */
        const bool use_extra_resources = KSystemControl::Init::ShouldIncreaseThreadResourceLimit();
        const size_t resource_region_size = KernelResourceRegionSize + (use_extra_resources ? ExtraKernelResourceSize : 0);

        /* Determine the size of the slab region. */
        const size_t slab_region_size = 0x647000; /* TODO: Calculate this on the fly. */
        MESOSPHERE_INIT_ABORT_UNLESS(slab_region_size <= resource_region_size);

        /* Setup the slab region. */
        const KPhysicalAddress code_start_phys_addr = ttbr1_table.GetPhysicalAddress(code_start_virt_addr);
        const KPhysicalAddress code_end_phys_addr   = code_start_phys_addr + (code_end_virt_addr - code_start_virt_addr);
        const KPhysicalAddress slab_start_phys_addr = code_end_phys_addr;
        const KPhysicalAddress slab_end_phys_addr   = slab_start_phys_addr + slab_region_size;
        constexpr size_t SlabRegionAlign = KernelAslrAlignment;
        const size_t slab_region_needed_size = util::AlignUp(GetInteger(code_end_phys_addr) + slab_region_size, SlabRegionAlign) - util::AlignDown(GetInteger(code_end_phys_addr), SlabRegionAlign);
        const KVirtualAddress slab_region_start = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegion(slab_region_needed_size, SlabRegionAlign, KMemoryRegionType_Kernel) + (GetInteger(code_end_phys_addr) % SlabRegionAlign);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(slab_region_start), slab_region_size, KMemoryRegionType_KernelSlab));

        /* Set the slab region's pair block. */
        KMemoryLayout::GetVirtualMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_KernelSlab)->SetPairAddress(GetInteger(slab_start_phys_addr));

        /* Setup the temp region. */
        constexpr size_t TempRegionSize  = 128_MB;
        constexpr size_t TempRegionAlign = KernelAslrAlignment;
        const KVirtualAddress temp_region_start = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegion(TempRegionSize, TempRegionAlign, KMemoryRegionType_Kernel);
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(temp_region_start), TempRegionSize, KMemoryRegionType_KernelTemp));

        /* Setup the Misc Unknown Debug region, if it's not zero. */
        if (misc_unk_debug_phys_addr) {
            constexpr size_t MiscUnknownDebugRegionSize  = PageSize;
            constexpr size_t MiscUnknownDebugRegionAlign = PageSize;
            const KVirtualAddress misc_unk_debug_virt_addr = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegionWithGuard(MiscUnknownDebugRegionSize, MiscUnknownDebugRegionAlign, KMemoryRegionType_KernelMisc, PageSize);
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(misc_unk_debug_virt_addr), MiscUnknownDebugRegionSize, KMemoryRegionType_KernelMiscUnknownDebug));
            ttbr1_table.Map(misc_unk_debug_virt_addr, MiscUnknownDebugRegionSize, misc_unk_debug_phys_addr, KernelRoDataAttribute, g_initial_page_allocator);
        }

        /* Setup board-specific device physical blocks. */
        SetupDevicePhysicalMemoryBlocks();

        /* Automatically map in devices that have auto-map attributes. */
        for (auto &block : KMemoryLayout::GetPhysicalMemoryBlockTree()) {
            /* We only care about automatically-mapped blocks. */
            if (!block.IsDerivedFrom(KMemoryRegionType_KernelAutoMap)) {
                continue;
            }

            /* If this block has already been mapped, no need to consider it. */
            if (block.HasTypeAttribute(KMemoryRegionAttr_DidKernelMap)) {
                continue;
            }

            /* Set the attribute to note we've mapped this block. */
            block.SetTypeAttribute(KMemoryRegionAttr_DidKernelMap);

            /* Create a virtual pair block and insert it into the tree. */
            const KPhysicalAddress map_phys_addr = util::AlignDown(block.GetAddress(), PageSize);
            const size_t map_size = util::AlignUp(block.GetEndAddress(), PageSize) - GetInteger(map_phys_addr);
            const KVirtualAddress map_virt_addr = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegionWithGuard(map_size, PageSize, KMemoryRegionType_KernelMisc, PageSize);
            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(map_virt_addr), map_size, KMemoryRegionType_KernelMiscMappedDevice));
            block.SetPairAddress(GetInteger(map_virt_addr) + block.GetAddress() - GetInteger(map_phys_addr));

            /* Map the page in to our page table. */
            ttbr1_table.Map(map_virt_addr, map_size, map_phys_addr, KernelMmioAttribute, g_initial_page_allocator);
        }

        /* Setup the basic DRAM blocks. */
        SetupDramPhysicalMemoryBlocks();

        /* Insert a physical block for the kernel code region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryBlockTree().Insert(GetInteger(code_start_phys_addr), (code_end_virt_addr - code_start_virt_addr), KMemoryRegionType_DramKernelCode));
        KMemoryLayout::GetPhysicalMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_DramKernelCode)->SetPairAddress(code_start_virt_addr);

        /* Insert a physical block for the kernel slab region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryBlockTree().Insert(GetInteger(slab_start_phys_addr), slab_region_size, KMemoryRegionType_DramKernelSlab));
        KMemoryLayout::GetPhysicalMemoryBlockTree().FindFirstBlockByTypeAttr(KMemoryRegionType_DramKernelSlab)->SetPairAddress(GetInteger(slab_region_start));

        /* Map and clear the slab region. */
        ttbr1_table.Map(slab_region_start, slab_region_size, slab_start_phys_addr, KernelRwDataAttribute, g_initial_page_allocator);
        std::memset(GetVoidPointer(slab_region_start), 0, slab_region_size);

        /* Determine size available for kernel page table heaps, requiring > 8 MB. */
        const KPhysicalAddress resource_end_phys_addr = slab_start_phys_addr + resource_region_size;
        const size_t page_table_heap_size = GetInteger(resource_end_phys_addr) - GetInteger(slab_end_phys_addr);
        MESOSPHERE_INIT_ABORT_UNLESS(page_table_heap_size / 4_MB > 2);

        /* Insert a physical block for the kernel page table heap region */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryBlockTree().Insert(GetInteger(slab_end_phys_addr), page_table_heap_size, KMemoryRegionType_DramKernelPtHeap));

        /* All DRAM blocks that we haven't tagged by this point will be mapped under the linear mapping. Tag them. */
        for (auto &block : KMemoryLayout::GetPhysicalMemoryBlockTree()) {
            if (block.GetType() == KMemoryRegionType_Dram) {
                block.SetTypeAttribute(KMemoryRegionAttr_LinearMapped);
            }
        }

        /* Setup the linear mapping region. */
        constexpr size_t LinearRegionAlign = 1_GB;
        const auto linear_extents = KMemoryLayout::GetPhysicalMemoryBlockTree().GetDerivedRegionExtents(KMemoryRegionAttr_LinearMapped);
        const KPhysicalAddress aligned_linear_phys_start = util::AlignDown(linear_extents.first_block->GetAddress(), LinearRegionAlign);
        const size_t linear_region_size = util::AlignUp(linear_extents.last_block->GetEndAddress(), LinearRegionAlign) - GetInteger(aligned_linear_phys_start);
        const KVirtualAddress linear_region_start = KMemoryLayout::GetVirtualMemoryBlockTree().GetRandomAlignedRegionWithGuard(linear_region_size, LinearRegionAlign, KMemoryRegionType_None, LinearRegionAlign);

        const uintptr_t linear_region_phys_to_virt_diff = GetInteger(linear_region_start) - GetInteger(aligned_linear_phys_start);

        /* Map and create blocks for all the linearly-mapped data. */
        for (auto &block : KMemoryLayout::GetPhysicalMemoryBlockTree()) {
            if (!block.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
                continue;
            }

            const uintptr_t block_virt_addr = block.GetAddress() + linear_region_phys_to_virt_diff;
            ttbr1_table.Map(block_virt_addr, block.GetSize(), block.GetAddress(), KernelRwDataAttribute, g_initial_page_allocator);

            MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(block_virt_addr, block.GetSize(), GetTypeForVirtualLinearMapping(block.GetType())));
            block.SetPairAddress(block_virt_addr);
            KMemoryLayout::GetVirtualMemoryBlockTree().FindContainingBlock(block_virt_addr)->SetPairAddress(block.GetAddress());
        }

        /* Create blocks for and map all core-specific stacks. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            MapStackForCore(ttbr1_table, KMemoryRegionType_KernelMiscMainStack, i);
            MapStackForCore(ttbr1_table, KMemoryRegionType_KernelMiscIdleStack, i);
            MapStackForCore(ttbr1_table, KMemoryRegionType_KernelMiscExceptionStack, i);
        }

        /* Setup the KCoreLocalRegion blocks. */
        SetupCoreLocalRegionMemoryBlocks(ttbr1_table, g_initial_page_allocator);

        /* Finalize the page allocator, we're done allocating at this point. */
        const KPhysicalAddress final_init_page_table_end_address = g_initial_page_allocator.GetFinalNextAddress();
        const size_t init_page_table_region_size = GetInteger(final_init_page_table_end_address) - GetInteger(resource_end_phys_addr);

        /* Insert blocks for the initial page table region. */
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetPhysicalMemoryBlockTree().Insert(GetInteger(resource_end_phys_addr), init_page_table_region_size, KMemoryRegionType_DramKernelInitPt));
        MESOSPHERE_INIT_ABORT_UNLESS(KMemoryLayout::GetVirtualMemoryBlockTree().Insert(GetInteger(resource_end_phys_addr) + linear_region_phys_to_virt_diff, init_page_table_region_size, KMemoryRegionType_VirtualKernelInitPt));

        /* All linear-mapped DRAM blocks that we haven't tagged by this point will be allocated to some pool partition. Tag them. */
        for (auto &block : KMemoryLayout::GetPhysicalMemoryBlockTree()) {
            if (block.GetType() == KMemoryRegionType_DramLinearMapped) {
                block.SetType(KMemoryRegionType_DramPoolPartition);
            }
        }

        /* Setup all other memory blocks needed to arrange the pool partitions. */
        SetupPoolPartitionMemoryBlocks();

        /* Cache all linear blocks in their own trees for faster access, later. */
        KMemoryLayout::InitializeLinearMemoryBlockTrees(aligned_linear_phys_start, linear_region_start);

        /* Turn on all other cores. */
        TurnOnAllCores(GetInteger(ttbr1_table.GetPhysicalAddress(reinterpret_cast<uintptr_t>(::ams::kern::init::StartOtherCore))));
    }

    KPhysicalAddress GetInitArgumentsAddress(s32 core_id) {
        return g_init_arguments_phys_addr[core_id];
    }

    void SetInitArguments(s32 core_id, KPhysicalAddress address, uintptr_t arg) {
        KInitArguments *init_args = reinterpret_cast<KInitArguments *>(GetInteger(address));
        init_args->ttbr0            = cpu::GetTtbr0El1();
        init_args->ttbr1            = arg;
        init_args->tcr              = cpu::GetTcrEl1();
        init_args->mair             = cpu::GetMairEl1();
        init_args->cpuactlr         = cpu::GetCpuActlrEl1();
        init_args->cpuectlr         = cpu::GetCpuEctlrEl1();
        init_args->sctlr            = cpu::GetSctlrEl1();
        init_args->sp               = GetInteger(KMemoryLayout::GetMainStackTopAddress(core_id));
        init_args->entrypoint       = reinterpret_cast<uintptr_t>(::ams::kern::HorizonKernelMain);
        init_args->argument         = static_cast<u64>(core_id);
        init_args->setup_function   = reinterpret_cast<uintptr_t>(::ams::kern::init::StartOtherCore);
        g_init_arguments_phys_addr[core_id] = address;
    }


    void StoreInitArguments() {
        StoreDataCache(g_init_arguments_phys_addr, sizeof(g_init_arguments_phys_addr));
    }

    void InitializeDebugRegisters() {
        /* TODO */
    }

    void InitializeExceptionVectors() {
        /* TODO */
    }

}