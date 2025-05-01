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

namespace ams::kern {

    namespace {

        KDynamicPageManager g_resource_manager_page_manager;

        template<typename T>
        ALWAYS_INLINE void PrintMemoryRegion(const char *prefix, const T &extents) {
            static_assert(std::is_same<decltype(extents.GetAddress()),     uintptr_t>::value);
            static_assert(std::is_same<decltype(extents.GetLastAddress()), uintptr_t>::value);
            if constexpr (std::is_same<uintptr_t, unsigned int>::value) {
                MESOSPHERE_LOG("%-24s0x%08x - 0x%08x\n", prefix, extents.GetAddress(), extents.GetLastAddress());
            } else if constexpr (std::is_same<uintptr_t, unsigned long>::value) {
                MESOSPHERE_LOG("%-24s0x%016lx - 0x%016lx\n", prefix, extents.GetAddress(), extents.GetLastAddress());
            } else if constexpr (std::is_same<uintptr_t, unsigned long long>::value) {
                MESOSPHERE_LOG("%-24s0x%016llx - 0x%016llx\n", prefix, extents.GetAddress(), extents.GetLastAddress());
            } else {
                static_assert(!std::is_same<T, T>::value, "Unknown uintptr_t width!");
            }
        }

    }

    void Kernel::InitializeCoreLocalRegion(s32 core_id) {
        /* The core local region no longer exists, so just clear the current thread. */
        AMS_UNUSED(core_id);
        SetCurrentThread(nullptr);
    }

    void Kernel::InitializeMainAndIdleThreads(s32 core_id) {
        /* This function wants to setup the main thread and the idle thread. */
        KThread *main_thread       = std::addressof(Kernel::GetMainThread(core_id));
        void    *main_thread_stack = GetVoidPointer(KMemoryLayout::GetMainStackTopAddress(core_id));
        KThread *idle_thread       = std::addressof(Kernel::GetIdleThread(core_id));
        void    *idle_thread_stack = GetVoidPointer(KMemoryLayout::GetIdleStackTopAddress(core_id));
        KAutoObject::Create<KThread>(main_thread);
        KAutoObject::Create<KThread>(idle_thread);
        main_thread->Initialize(nullptr, 0, main_thread_stack, 0, KThread::MainThreadPriority, core_id, nullptr, KThread::ThreadType_Main);
        idle_thread->Initialize(nullptr, 0, idle_thread_stack, 0, KThread::IdleThreadPriority, core_id, nullptr, KThread::ThreadType_Main);

        /* Set the current thread to be the main thread, and we have no processes running yet. */
        SetCurrentThread(main_thread);

        /* Initialize the interrupt manager, hardware timer, and scheduler */
        GetInterruptManager().Initialize(core_id);
        GetHardwareTimer().Initialize();
        GetScheduler().Initialize(idle_thread);
    }

    void Kernel::InitializeResourceManagers(KVirtualAddress address, size_t size) {
        /* Ensure that the buffer is suitable for our use. */
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, PageSize));

        /* Ensure that we have space for our reference counts. */
        const size_t rc_size = util::AlignUp(KPageTableSlabHeap::CalculateReferenceCountSize(size), PageSize);
        MESOSPHERE_ABORT_UNLESS(rc_size < size);
        size -= rc_size;

        /* Determine dynamic page managers. */
        KDynamicPageManager * const app_dynamic_page_manager = nullptr;
        KDynamicPageManager * const sys_dynamic_page_manager = KTargetSystem::IsDynamicResourceLimitsEnabled() ? std::addressof(g_resource_manager_page_manager) : nullptr;

        /* Initialize the resource managers' shared page manager. */
        g_resource_manager_page_manager.Initialize(address, size, std::max<size_t>(PageSize, KPageBufferSlabHeap::BufferSize));

        /* Initialize the KPageBuffer slab heap. */
        KPageBuffer::InitializeSlabHeap(g_resource_manager_page_manager);

        /* Initialize the block info heap. */
        s_block_info_heap.Initialize(std::addressof(g_resource_manager_page_manager), BlockInfoSlabHeapSize);

        /* Initialize the system slab managers. */
        s_sys_block_info_manager.Initialize(sys_dynamic_page_manager, std::addressof(s_block_info_heap));
        s_sys_memory_block_heap.Initialize(std::addressof(g_resource_manager_page_manager), SystemMemoryBlockSlabHeapSize);
        s_sys_memory_block_manager.Initialize(sys_dynamic_page_manager, std::addressof(s_sys_memory_block_heap));

        /* Initialize the application slab managers. */
        s_app_block_info_manager.Initialize(app_dynamic_page_manager, std::addressof(s_block_info_heap));
        s_app_memory_block_heap.Initialize(std::addressof(g_resource_manager_page_manager), ApplicationMemoryBlockSlabHeapSize);
        s_app_memory_block_manager.Initialize(app_dynamic_page_manager, std::addressof(s_app_memory_block_heap));

        /* Reserve all but a fixed number of remaining pages for the page table heap. */
        const size_t num_pt_pages = g_resource_manager_page_manager.GetCount() - g_resource_manager_page_manager.GetUsed() - ReservedDynamicPageCount;
        s_page_table_heap.Initialize(std::addressof(g_resource_manager_page_manager), num_pt_pages, GetPointer<KPageTableManager::RefCount>(address + size));

        /* Create and initialize the system system resource. */
        s_sys_page_table_manager.Initialize(sys_dynamic_page_manager, std::addressof(s_page_table_heap));
        KAutoObject::Create<KSystemResource>(std::addressof(s_sys_system_resource));
        s_sys_system_resource.SetManagers(s_sys_memory_block_manager, s_sys_block_info_manager, s_sys_page_table_manager);

        /* Create and initialize the application system resource. */
        s_app_page_table_manager.Initialize(app_dynamic_page_manager, std::addressof(s_page_table_heap));
        KAutoObject::Create<KSystemResource>(std::addressof(s_app_system_resource));
        s_app_system_resource.SetManagers(s_app_memory_block_manager, s_app_block_info_manager, s_app_page_table_manager);

        /* Check that we have the correct number of dynamic pages available. */
        MESOSPHERE_ABORT_UNLESS(g_resource_manager_page_manager.GetCount() - g_resource_manager_page_manager.GetUsed() == ReservedDynamicPageCount);
    }

    void Kernel::PrintLayout() {
        const auto target_fw = kern::GetTargetFirmware();

        /* Print out the kernel version. */
        MESOSPHERE_LOG("Horizon Kernel (Mesosphere)\n");
        MESOSPHERE_LOG("Built:                  %s %s\n", __DATE__, __TIME__);
        MESOSPHERE_LOG("Atmosphere version:     %d.%d.%d-%s\n", ATMOSPHERE_RELEASE_VERSION, ATMOSPHERE_GIT_REVISION);
        MESOSPHERE_LOG("Target Firmware:        %d.%d.%d\n", (target_fw >> 24) & 0xFF, (target_fw >> 16) & 0xFF, (target_fw >> 8) & 0xFF);
        MESOSPHERE_LOG("Supported OS version:   %d.%d.%d\n", ATMOSPHERE_SUPPORTED_HOS_VERSION_MAJOR, ATMOSPHERE_SUPPORTED_HOS_VERSION_MINOR, ATMOSPHERE_SUPPORTED_HOS_VERSION_MICRO);
        MESOSPHERE_LOG("\n");

        /* Print relative memory usage. */
        const auto [total, kernel] = KMemoryLayout::GetTotalAndKernelMemorySizes();
        MESOSPHERE_LOG("Kernel Memory Usage:    %zu/%zu MB\n", util::AlignUp(kernel, 1_MB) / 1_MB, util::AlignUp(total, 1_MB) / 1_MB);
        MESOSPHERE_LOG("\n");

        /* Print out important memory layout regions. */
        MESOSPHERE_LOG("Virtual Memory Layout\n");
        PrintMemoryRegion("    KernelRegion",       KMemoryLayout::GetKernelRegionExtents());
        PrintMemoryRegion("        Code",           KMemoryLayout::GetKernelCodeRegionExtents());
        PrintMemoryRegion("        Stack",          KMemoryLayout::GetKernelStackRegionExtents());
        PrintMemoryRegion("        Misc",           KMemoryLayout::GetKernelMiscRegionExtents());
        PrintMemoryRegion("        Slab",           KMemoryLayout::GetKernelSlabRegionExtents());
        PrintMemoryRegion("    LinearRegion",       KMemoryLayout::GetLinearRegionVirtualExtents());
        MESOSPHERE_LOG("\n");

        MESOSPHERE_LOG("Physical Memory Layout\n");
        PrintMemoryRegion("    LinearRegion",       KMemoryLayout::GetLinearRegionPhysicalExtents());
        PrintMemoryRegion("    CarveoutRegion",     KMemoryLayout::GetCarveoutRegionExtents());
        MESOSPHERE_LOG("\n");
        PrintMemoryRegion("    KernelRegion",       KMemoryLayout::GetKernelRegionPhysicalExtents());
        PrintMemoryRegion("        Code",           KMemoryLayout::GetKernelCodeRegionPhysicalExtents());
        PrintMemoryRegion("        Slab",           KMemoryLayout::GetKernelSlabRegionPhysicalExtents());
        if constexpr (KSystemControl::SecureAppletMemorySize > 0) {
            PrintMemoryRegion("        SecureApplet", KMemoryLayout::GetKernelSecureAppletMemoryRegionPhysicalExtents());
        }
        PrintMemoryRegion("        PageTableHeap",  KMemoryLayout::GetKernelPageTableHeapRegionPhysicalExtents());
        PrintMemoryRegion("        InitPageTable",  KMemoryLayout::GetKernelInitPageTableRegionPhysicalExtents());
        if constexpr (IsKTraceEnabled) {
            MESOSPHERE_LOG("    DebugRegion\n");
            PrintMemoryRegion("        Trace Buffer",   KMemoryLayout::GetKernelTraceBufferRegionPhysicalExtents());
        }
        PrintMemoryRegion("    MemoryPoolRegion",   KMemoryLayout::GetKernelPoolPartitionRegionPhysicalExtents());
        if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
            PrintMemoryRegion("        Management",     KMemoryLayout::GetKernelPoolManagementRegionPhysicalExtents());
            PrintMemoryRegion("        System",         KMemoryLayout::GetKernelSystemPoolRegionPhysicalExtents());
            if (KMemoryLayout::HasKernelSystemNonSecurePoolRegion()) {
                PrintMemoryRegion("        SystemUnsafe",   KMemoryLayout::GetKernelSystemNonSecurePoolRegionPhysicalExtents());
            }
            if (KMemoryLayout::HasKernelAppletPoolRegion()) {
                PrintMemoryRegion("        Applet",         KMemoryLayout::GetKernelAppletPoolRegionPhysicalExtents());
            }
            if (KMemoryLayout::HasKernelApplicationPoolRegion()) {
                PrintMemoryRegion("        Application",    KMemoryLayout::GetKernelApplicationPoolRegionPhysicalExtents());
            }
        } else {
            PrintMemoryRegion("        Management", KMemoryLayout::GetKernelPoolManagementRegionPhysicalExtents());
            PrintMemoryRegion("        Secure",     KMemoryLayout::GetKernelSystemPoolRegionPhysicalExtents());
            PrintMemoryRegion("        Unsafe",     KMemoryLayout::GetKernelApplicationPoolRegionPhysicalExtents());
        }
        MESOSPHERE_LOG("\n");
    }

}
