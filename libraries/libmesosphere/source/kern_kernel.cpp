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

namespace ams::kern {

    /* Declare kernel data members in kernel TU. */
    Kernel::State           Kernel::s_state = Kernel::State::Invalid;
    KThread                 Kernel::s_main_threads[cpu::NumCores];
    KThread                 Kernel::s_idle_threads[cpu::NumCores];
    KResourceLimit          Kernel::s_system_resource_limit;
    KMemoryManager          Kernel::s_memory_manager;
    KPageTableManager       Kernel::s_page_table_manager;
    KMemoryBlockSlabManager Kernel::s_app_memory_block_manager;
    KMemoryBlockSlabManager Kernel::s_sys_memory_block_manager;
    KBlockInfoManager       Kernel::s_block_info_manager;

    void Kernel::InitializeCoreLocalRegion(s32 core_id) {
        /* Construct the core local region object in place. */
        KCoreLocalContext *clc = GetPointer<KCoreLocalContext>(KMemoryLayout::GetCoreLocalRegionAddress());
        new (clc) KCoreLocalContext;

        /* Set the core local region address into the global register. */
        cpu::SetCoreLocalRegionAddress(reinterpret_cast<uintptr_t>(clc));

        /* Initialize current context. */
        clc->current.current_thread = nullptr;
        clc->current.current_process = nullptr;
        clc->current.scheduler = std::addressof(clc->scheduler);
        clc->current.interrupt_task_manager = std::addressof(clc->interrupt_task_manager);
        clc->current.core_id = core_id;
        clc->current.exception_stack_top = GetVoidPointer(KMemoryLayout::GetExceptionStackTopAddress(core_id) - sizeof(KThread::StackParameters));

        /* Clear debugging counters. */
        clc->num_sw_interrupts = 0;
        clc->num_hw_interrupts = 0;
        clc->num_svc = 0;
        clc->num_process_switches = 0;
        clc->num_thread_switches = 0;
        clc->num_fpu_switches = 0;

        for (size_t i = 0; i < util::size(clc->perf_counters); i++) {
            clc->perf_counters[i] = 0;
        }
    }

    void Kernel::InitializeMainAndIdleThreads(s32 core_id) {
        /* This function wants to setup the main thread and the idle thread. */
        KThread *main_thread       = std::addressof(Kernel::GetMainThread(core_id));
        void    *main_thread_stack = GetVoidPointer(KMemoryLayout::GetMainStackTopAddress(core_id));
        KThread *idle_thread       = std::addressof(Kernel::GetIdleThread(core_id));
        void    *idle_thread_stack = GetVoidPointer(KMemoryLayout::GetIdleStackTopAddress(core_id));
        KAutoObject::Create(main_thread);
        KAutoObject::Create(idle_thread);
        main_thread->Initialize(nullptr, 0, main_thread_stack, 0, KThread::MainThreadPriority, core_id, nullptr, KThread::ThreadType_Main);
        idle_thread->Initialize(nullptr, 0, idle_thread_stack, 0, KThread::IdleThreadPriority, core_id, nullptr, KThread::ThreadType_Main);

        /* Set the current thread to be the main thread, and we have no processes running yet. */
        SetCurrentThread(main_thread);
        SetCurrentProcess(nullptr);

        /* Initialize the interrupt manager, hardware timer, and scheduler */
        GetInterruptManager().Initialize(core_id);
        GetHardwareTimer().Initialize(core_id);
        GetScheduler().Initialize(idle_thread);
    }

    void Kernel::InitializeResourceManagers(KVirtualAddress address, size_t size) {
        /* Ensure that the buffer is suitable for our use. */
        const size_t app_size   = ApplicationMemoryBlockSlabHeapSize * sizeof(KMemoryBlock);
        const size_t sys_size   = SystemMemoryBlockSlabHeapSize      * sizeof(KMemoryBlock);
        const size_t info_size  = BlockInfoSlabHeapSize              * sizeof(KBlockInfo);
        const size_t fixed_size = util::AlignUp(app_size + sys_size + info_size, PageSize);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(address), PageSize));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, PageSize));
        MESOSPHERE_ABORT_UNLESS(fixed_size < size);

        size_t pt_size = size - fixed_size;
        const size_t rc_size = util::AlignUp(KPageTableManager::CalculateReferenceCountSize(pt_size), PageSize);
        MESOSPHERE_ABORT_UNLESS(rc_size < pt_size);
        pt_size -= rc_size;

        /* Initialize the slabheaps. */
        s_app_memory_block_manager.Initialize(address + pt_size, app_size);
        s_sys_memory_block_manager.Initialize(address + pt_size + app_size, sys_size);
        s_block_info_manager.Initialize(address + pt_size + app_size + sys_size, info_size);
        s_page_table_manager.Initialize(address, pt_size, GetPointer<KPageTableManager::RefCount>(address + pt_size + fixed_size));
    }

}
