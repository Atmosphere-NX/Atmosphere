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

    namespace {

        constexpr u64 InitialProcessIdMin = 1;
        constexpr u64 InitialProcessIdMax = 0x50;

        constexpr u64 ProcessIdMin        = InitialProcessIdMax + 1;
        constexpr u64 ProcessIdMax        = std::numeric_limits<u64>::max();

        std::atomic<u64> g_initial_process_id = InitialProcessIdMin;
        std::atomic<u64> g_process_id         = ProcessIdMin;

        void TerminateChildren(KProcess *process, const KThread *thread_to_not_terminate) {
            /* Request that all children threads terminate. */
            {
                KScopedLightLock proc_lk(process->GetListLock());
                KScopedSchedulerLock sl;

                auto &thread_list = process->GetThreadList();
                for (auto it = thread_list.begin(); it != thread_list.end(); ++it) {
                    if (KThread *thread = std::addressof(*it); thread != thread_to_not_terminate) {
                        if (thread->GetState() != KThread::ThreadState_Terminated) {
                            thread->RequestTerminate();
                        }
                    }
                }
            }

            /* Wait for all children threads to terminate.*/
            while (true) {
                /* Get the next child. */
                KThread *cur_child = nullptr;
                {
                    KScopedLightLock proc_lk(process->GetListLock());

                    auto &thread_list = process->GetThreadList();
                    for (auto it = thread_list.begin(); it != thread_list.end(); ++it) {
                        if (KThread *thread = std::addressof(*it); thread != thread_to_not_terminate) {
                            if (thread->GetState() != KThread::ThreadState_Terminated) {
                                if (AMS_LIKELY(thread->Open())) {
                                    cur_child = thread;
                                    break;
                                }
                            }
                        }
                    }
                }

                /* If we didn't find any non-terminated children, we're done. */
                if (cur_child == nullptr) {
                    break;
                }

                /* Terminate and close the thread. */
                cur_child->Terminate();
                cur_child->Close();
            }
        }

    }

    void KProcess::Finalize() {
        /* Delete the process local region. */
        this->DeleteThreadLocalRegion(m_plr_address);

        /* Get the used memory size. */
        const size_t used_memory_size = this->GetUsedUserPhysicalMemorySize();

        /* Finalize the page table. */
        m_page_table.Finalize();

        /* Free the system resource. */
        if (m_system_resource_address != Null<KVirtualAddress>) {
            /* Check that we have no outstanding allocations. */
            MESOSPHERE_ABORT_UNLESS(m_memory_block_slab_manager.GetUsed() == 0);
            MESOSPHERE_ABORT_UNLESS(m_block_info_manager.GetUsed()        == 0);
            MESOSPHERE_ABORT_UNLESS(m_page_table_manager.GetUsed()        == 0);

            /* Free the memory. */
            KSystemControl::FreeSecureMemory(m_system_resource_address, m_system_resource_num_pages * PageSize, m_memory_pool);

            /* Clear our tracking variables. */
            m_system_resource_address   = Null<KVirtualAddress>;
            m_system_resource_num_pages = 0;

            /* Finalize optimized memory. If memory wasn't optimized, this is a no-op. */
            Kernel::GetMemoryManager().FinalizeOptimizedMemory(this->GetId(), m_memory_pool);
        }

        /* Release memory to the resource limit. */
        if (m_resource_limit != nullptr) {
            MESOSPHERE_ABORT_UNLESS(used_memory_size >= m_memory_release_hint);
            m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, used_memory_size, used_memory_size - m_memory_release_hint);
            m_resource_limit->Close();
        }

        /* Free all shared memory infos. */
        {
            auto it = m_shared_memory_list.begin();
            while (it != m_shared_memory_list.end()) {
                KSharedMemoryInfo *info = std::addressof(*it);
                KSharedMemory *shmem    = info->GetSharedMemory();

                while (!info->Close()) {
                    shmem->Close();
                }
                shmem->Close();

                it = m_shared_memory_list.erase(it);
                KSharedMemoryInfo::Free(info);
            }
        }

        /* Close all references to our betas. */
        {
            auto it = m_beta_list.begin();
            while (it != m_beta_list.end()) {
                KBeta *beta = std::addressof(*it);
                it = m_beta_list.erase(it);

                beta->Close();
            }
        }

        /* Our thread local page list must be empty at this point. */
        MESOSPHERE_ABORT_UNLESS(m_partially_used_tlp_tree.empty());
        MESOSPHERE_ABORT_UNLESS(m_fully_used_tlp_tree.empty());

        /* Log that we finalized for debug. */
        MESOSPHERE_LOG("KProcess::Finalize() pid=%ld name=%-12s\n", m_process_id, m_name);

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KProcess, KSynchronizationObject>::Finalize();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params) {
        /* Validate that the intended kernel version is high enough for us to support. */
        R_UNLESS(m_capabilities.GetIntendedKernelVersion() >= ams::svc::RequiredKernelVersion,  svc::ResultInvalidCombination());

        /* Validate that the intended kernel version isn't too high for us to support. */
        R_UNLESS(m_capabilities.GetIntendedKernelVersion() <= ams::svc::SupportedKernelVersion, svc::ResultInvalidCombination());

        /* Create and clear the process local region. */
        R_TRY(this->CreateThreadLocalRegion(std::addressof(m_plr_address)));
        m_plr_heap_address = this->GetThreadLocalRegionPointer(m_plr_address);
        std::memset(m_plr_heap_address, 0, ams::svc::ThreadLocalRegionSize);

        /* Copy in the name from parameters. */
        static_assert(sizeof(params.name) < sizeof(m_name));
        std::memcpy(m_name, params.name, sizeof(params.name));
        m_name[sizeof(params.name)] = 0;

        /* Set misc fields. */
        m_state                     = State_Created;
        m_main_thread_stack_size    = 0;
        m_creation_time             = KHardwareTimer::GetTick();
        m_used_kernel_memory_size   = 0;
        m_ideal_core_id             = 0;
        m_flags                     = params.flags;
        m_version                   = params.version;
        m_program_id                = params.program_id;
        m_code_address              = params.code_address;
        m_code_size                 = params.code_num_pages * PageSize;
        m_is_application            = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
        m_is_jit_debug              = false;

        /* Set thread fields. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            m_running_threads[i]            = nullptr;
            m_running_thread_idle_counts[i] = 0;
            m_pinned_threads[i]             = nullptr;
        }

        /* Set max memory based on address space type. */
        switch ((params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask)) {
            case ams::svc::CreateProcessFlag_AddressSpace32Bit:
            case ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated:
            case ams::svc::CreateProcessFlag_AddressSpace64Bit:
                m_max_process_memory = m_page_table.GetHeapRegionSize();
                break;
            case ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                m_max_process_memory = m_page_table.GetHeapRegionSize() + m_page_table.GetAliasRegionSize();
                break;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }

        /* Generate random entropy. */
        KSystemControl::GenerateRandomBytes(m_entropy, sizeof(m_entropy));

        /* Clear remaining fields. */
        m_num_threads           = 0;
        m_peak_num_threads      = 0;
        m_num_created_threads   = 0;
        m_num_process_switches  = 0;
        m_num_thread_switches   = 0;
        m_num_fpu_switches      = 0;
        m_num_supervisor_calls  = 0;
        m_num_ipc_messages      = 0;

        m_is_signaled           = false;
        m_attached_object       = nullptr;
        m_exception_thread      = nullptr;
        m_is_suspended          = false;
        m_memory_release_hint   = 0;
        m_schedule_count        = 0;

        /* We're initialized! */
        m_is_initialized = true;

        return ResultSuccess();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);
        MESOSPHERE_ABORT_UNLESS((params.code_num_pages * PageSize) / PageSize == static_cast<size_t>(params.code_num_pages));

        /* Set members. */
        m_memory_pool               = pool;
        m_resource_limit            = res_limit;
        m_system_resource_address   = Null<KVirtualAddress>;
        m_system_resource_num_pages = 0;

        /* Setup page table. */
        /* NOTE: Nintendo passes process ID despite not having set it yet. */
        /* This goes completely unused, but even so... */
        {
            const auto as_type          = static_cast<ams::svc::CreateProcessFlag>(params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask);
            const bool enable_aslr      = (params.flags & ams::svc::CreateProcessFlag_EnableAslr) != 0;
            const bool enable_das_merge = (params.flags & ams::svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge) == 0;
            const bool is_app           = (params.flags & ams::svc::CreateProcessFlag_IsApplication) != 0;
            auto *mem_block_manager     = std::addressof(is_app ? Kernel::GetApplicationMemoryBlockManager() : Kernel::GetSystemMemoryBlockManager());
            auto *block_info_manager    = std::addressof(Kernel::GetBlockInfoManager());
            auto *pt_manager            = std::addressof(Kernel::GetPageTableManager());
            R_TRY(m_page_table.Initialize(m_process_id, as_type, enable_aslr, enable_das_merge, !enable_aslr, pool, params.code_address, params.code_num_pages * PageSize, mem_block_manager, block_info_manager, pt_manager));
        }
        auto pt_guard = SCOPE_GUARD { m_page_table.Finalize(); };

        /* Ensure we can insert the code region. */
        R_UNLESS(m_page_table.CanContain(params.code_address, params.code_num_pages * PageSize, KMemoryState_Code), svc::ResultInvalidMemoryRegion());

        /* Map the code region. */
        R_TRY(m_page_table.MapPageGroup(params.code_address, pg, KMemoryState_Code, KMemoryPermission_KernelRead));

        /* Initialize capabilities. */
        R_TRY(m_capabilities.Initialize(caps, num_caps, std::addressof(m_page_table)));

        /* Initialize the process id. */
        m_process_id = g_initial_process_id++;
        MESOSPHERE_ABORT_UNLESS(InitialProcessIdMin <= m_process_id);
        MESOSPHERE_ABORT_UNLESS(m_process_id <= InitialProcessIdMax);

        /* Initialize the rest of the process. */
        R_TRY(this->Initialize(params));

        /* Open a reference to the resource limit. */
        m_resource_limit->Open();

        /* We succeeded! */
        pt_guard.Cancel();
        return ResultSuccess();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);

        /* Set pool and resource limit. */
        m_memory_pool               = pool;
        m_resource_limit            = res_limit;

        /* Get the memory sizes. */
        const size_t code_num_pages            = params.code_num_pages;
        const size_t system_resource_num_pages = params.system_resource_num_pages;
        const size_t code_size                 = code_num_pages * PageSize;
        const size_t system_resource_size      = system_resource_num_pages * PageSize;

        /* Reserve memory for the system resource. */
        KScopedResourceReservation memory_reservation(this, ams::svc::LimitableResource_PhysicalMemoryMax, code_size + KSystemControl::CalculateRequiredSecureMemorySize(system_resource_size, pool));
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Setup page table resource objects. */
        KMemoryBlockSlabManager *mem_block_manager;
        KBlockInfoManager *block_info_manager;
        KPageTableManager *pt_manager;

        m_system_resource_address   = Null<KVirtualAddress>;
        m_system_resource_num_pages = 0;

        if (system_resource_num_pages != 0) {
            /* Allocate secure memory. */
            R_TRY(KSystemControl::AllocateSecureMemory(std::addressof(m_system_resource_address), system_resource_size, pool));

            /* Set the number of system resource pages. */
            MESOSPHERE_ASSERT(m_system_resource_address != Null<KVirtualAddress>);
            m_system_resource_num_pages = system_resource_num_pages;

            /* Initialize managers. */
            const size_t rc_size = util::AlignUp(KPageTableManager::CalculateReferenceCountSize(system_resource_size), PageSize);
            m_dynamic_page_manager.Initialize(m_system_resource_address + rc_size, system_resource_size - rc_size);
            m_page_table_manager.Initialize(std::addressof(m_dynamic_page_manager), GetPointer<KPageTableManager::RefCount>(m_system_resource_address));
            m_memory_block_slab_manager.Initialize(std::addressof(m_dynamic_page_manager));
            m_block_info_manager.Initialize(std::addressof(m_dynamic_page_manager));

            mem_block_manager  = std::addressof(m_memory_block_slab_manager);
            block_info_manager = std::addressof(m_block_info_manager);
            pt_manager         = std::addressof(m_page_table_manager);
        } else {
            const bool is_app  = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
            mem_block_manager  = std::addressof(is_app ? Kernel::GetApplicationMemoryBlockManager() : Kernel::GetSystemMemoryBlockManager());
            block_info_manager = std::addressof(Kernel::GetBlockInfoManager());
            pt_manager         = std::addressof(Kernel::GetPageTableManager());
        }

        /* Ensure we don't leak any secure memory we allocated. */
        auto sys_resource_guard = SCOPE_GUARD {
            if (m_system_resource_address != Null<KVirtualAddress>) {
                /* Check that we have no outstanding allocations. */
                MESOSPHERE_ABORT_UNLESS(m_memory_block_slab_manager.GetUsed() == 0);
                MESOSPHERE_ABORT_UNLESS(m_block_info_manager.GetUsed()        == 0);
                MESOSPHERE_ABORT_UNLESS(m_page_table_manager.GetUsed()        == 0);

                /* Free the memory. */
                KSystemControl::FreeSecureMemory(m_system_resource_address, system_resource_size, pool);

                /* Clear our tracking variables. */
                m_system_resource_address   = Null<KVirtualAddress>;
                m_system_resource_num_pages = 0;
            }
        };

        /* Setup page table. */
        /* NOTE: Nintendo passes process ID despite not having set it yet. */
        /* This goes completely unused, but even so... */
        {
            const auto as_type          = static_cast<ams::svc::CreateProcessFlag>(params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask);
            const bool enable_aslr      = (params.flags & ams::svc::CreateProcessFlag_EnableAslr) != 0;
            const bool enable_das_merge = (params.flags & ams::svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge) == 0;
            R_TRY(m_page_table.Initialize(m_process_id, as_type, enable_aslr, enable_das_merge, !enable_aslr, pool, params.code_address, code_size, mem_block_manager, block_info_manager, pt_manager));
        }
        auto pt_guard = SCOPE_GUARD { m_page_table.Finalize(); };

        /* Ensure we can insert the code region. */
        R_UNLESS(m_page_table.CanContain(params.code_address, code_size, KMemoryState_Code), svc::ResultInvalidMemoryRegion());

        /* Map the code region. */
        R_TRY(m_page_table.MapPages(params.code_address, code_num_pages, KMemoryState_Code, static_cast<KMemoryPermission>(KMemoryPermission_KernelRead | KMemoryPermission_NotMapped)));

        /* Initialize capabilities. */
        R_TRY(m_capabilities.Initialize(user_caps, num_caps, std::addressof(m_page_table)));

        /* Initialize the process id. */
        m_process_id = g_process_id++;
        MESOSPHERE_ABORT_UNLESS(ProcessIdMin <= m_process_id);
        MESOSPHERE_ABORT_UNLESS(m_process_id <= ProcessIdMax);

        /* If we should optimize memory allocations, do so. */
        if (m_system_resource_address != Null<KVirtualAddress> && (params.flags & ams::svc::CreateProcessFlag_OptimizeMemoryAllocation) != 0) {
            R_TRY(Kernel::GetMemoryManager().InitializeOptimizedMemory(m_process_id, pool));
        }

        /* Initialize the rest of the process. */
        R_TRY(this->Initialize(params));

        /* Open a reference to the resource limit. */
        m_resource_limit->Open();

        /* We succeeded, so commit our memory reservation and cancel our guards. */
        sys_resource_guard.Cancel();
        pt_guard.Cancel();
        memory_reservation.Commit();

        return ResultSuccess();
    }

    void KProcess::DoWorkerTask() {
        /* Terminate child threads. */
        TerminateChildren(this, nullptr);

        /* Call the debug callback. */
        KDebug::OnExitProcess(this);

        /* Finish termination. */
        this->FinishTermination();
    }

    void KProcess::StartTermination() {
        /* Terminate child threads other than the current one. */
        TerminateChildren(this, GetCurrentThreadPointer());

        /* Finalize the handle tahble. */
        m_handle_table.Finalize();
    }

    void KProcess::FinishTermination() {
        /* Release resource limit hint. */
        if (m_resource_limit != nullptr) {
            m_memory_release_hint = this->GetUsedUserPhysicalMemorySize();
            m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, 0, m_memory_release_hint);
        }

        /* Change state. */
        {
            KScopedSchedulerLock sl;
            this->ChangeState(State_Terminated);
        }

        /* Close. */
        this->Close();
    }

    void KProcess::Exit() {
        MESOSPHERE_ASSERT_THIS();

        /* Determine whether we need to start terminating */
        bool needs_terminate = false;
        {
            KScopedLightLock lk(m_state_lock);
            KScopedSchedulerLock sl;

            MESOSPHERE_ASSERT(m_state != State_Created);
            MESOSPHERE_ASSERT(m_state != State_CreatedAttached);
            MESOSPHERE_ASSERT(m_state != State_Crashed);
            MESOSPHERE_ASSERT(m_state != State_Terminated);
            if (m_state == State_Running || m_state == State_RunningAttached || m_state == State_DebugBreak) {
                this->ChangeState(State_Terminating);
                needs_terminate = true;
            }
        }

        /* If we need to start termination, do so. */
        if (needs_terminate) {
            this->StartTermination();

            /* Note for debug that we're exiting the process. */
            MESOSPHERE_LOG("KProcess::Exit() pid=%ld name=%-12s\n", m_process_id, m_name);

            /* Register the process as a work task. */
            KWorkerTaskManager::AddTask(KWorkerTaskManager::WorkerType_Exit, this);
        }

        /* Exit the current thread. */
        GetCurrentThread().Exit();
        MESOSPHERE_PANIC("Thread survived call to exit");
    }

    Result KProcess::Terminate() {
        MESOSPHERE_ASSERT_THIS();

        /* Determine whether we need to start terminating */
        bool needs_terminate = false;
        {
            KScopedLightLock lk(m_state_lock);

            /* Check whether we're allowed to terminate. */
            R_UNLESS(m_state != State_Created,         svc::ResultInvalidState());
            R_UNLESS(m_state != State_CreatedAttached, svc::ResultInvalidState());

            KScopedSchedulerLock sl;

            if (m_state == State_Running || m_state == State_RunningAttached || m_state == State_Crashed || m_state == State_DebugBreak) {
                this->ChangeState(State_Terminating);
                needs_terminate = true;
            }
        }

        /* If we need to terminate, do so. */
        if (needs_terminate) {
            /* Start termination. */
            this->StartTermination();

            /* Note for debug that we're terminating the process. */
            MESOSPHERE_LOG("KProcess::Terminate() pid=%ld name=%-12s\n", m_process_id, m_name);

            /* Call the debug callback. */
            KDebug::OnTerminateProcess(this);

            /* Finish termination. */
            this->FinishTermination();
        }

        return ResultSuccess();
    }

    Result KProcess::AddSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size) {
        /* Lock ourselves, to prevent concurrent access. */
        KScopedLightLock lk(m_state_lock);

        /* Address and size parameters aren't used. */
        MESOSPHERE_UNUSED(address, size);

        /* Try to find an existing info for the memory. */
        KSharedMemoryInfo *info = nullptr;
        for (auto it = m_shared_memory_list.begin(); it != m_shared_memory_list.end(); ++it) {
            if (it->GetSharedMemory() == shmem) {
                info = std::addressof(*it);
                break;
            }
        }

        /* If we didn't find an info, create one. */
        if (info == nullptr) {
            /* Allocate a new info. */
            info = KSharedMemoryInfo::Allocate();
            R_UNLESS(info != nullptr, svc::ResultOutOfResource());

            /* Initialize the info and add it to our list. */
            info->Initialize(shmem);
            m_shared_memory_list.push_back(*info);
        }

        /* Open a reference to the shared memory and its info. */
        shmem->Open();
        info->Open();

        return ResultSuccess();
    }

    void KProcess::RemoveSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size) {
        /* Lock ourselves, to prevent concurrent access. */
        KScopedLightLock lk(m_state_lock);

        /* Address and size parameters aren't used. */
        MESOSPHERE_UNUSED(address, size);

        /* Find an existing info for the memory. */
        KSharedMemoryInfo *info = nullptr;
        auto it = m_shared_memory_list.begin();
        for (/* ... */; it != m_shared_memory_list.end(); ++it) {
            if (it->GetSharedMemory() == shmem) {
                info = std::addressof(*it);
                break;
            }
        }
        MESOSPHERE_ABORT_UNLESS(info != nullptr);

        /* Close a reference to the info and its memory. */
        if (info->Close()) {
            m_shared_memory_list.erase(it);
            KSharedMemoryInfo::Free(info);
        }

        shmem->Close();
    }

    Result KProcess::CreateThreadLocalRegion(KProcessAddress *out) {
        KThreadLocalPage *tlp = nullptr;
        KProcessAddress   tlr = Null<KProcessAddress>;

        /* See if we can get a region from a partially used TLP. */
        {
            KScopedSchedulerLock sl;

            if (auto it = m_partially_used_tlp_tree.begin(); it != m_partially_used_tlp_tree.end()) {
                tlr = it->Reserve();
                MESOSPHERE_ABORT_UNLESS(tlr != Null<KProcessAddress>);

                if (it->IsAllUsed()) {
                    tlp = std::addressof(*it);
                    m_partially_used_tlp_tree.erase(it);
                    m_fully_used_tlp_tree.insert(*tlp);
                }

                *out = tlr;
                return ResultSuccess();
            }
        }

        /* Allocate a new page. */
        tlp = KThreadLocalPage::Allocate();
        R_UNLESS(tlp != nullptr, svc::ResultOutOfMemory());
        auto tlp_guard = SCOPE_GUARD { KThreadLocalPage::Free(tlp); };

        /* Initialize the new page. */
        R_TRY(tlp->Initialize(this));

        /* Reserve a TLR. */
        tlr = tlp->Reserve();
        MESOSPHERE_ABORT_UNLESS(tlr != Null<KProcessAddress>);

        /* Insert into our tree. */
        {
            KScopedSchedulerLock sl;
            if (tlp->IsAllUsed()) {
                m_fully_used_tlp_tree.insert(*tlp);
            } else {
                m_partially_used_tlp_tree.insert(*tlp);
            }
        }

        /* We succeeded! */
        tlp_guard.Cancel();
        *out = tlr;
        return ResultSuccess();
    }

    Result KProcess::DeleteThreadLocalRegion(KProcessAddress addr) {
        KThreadLocalPage *page_to_free = nullptr;

        /* Release the region. */
        {
            KScopedSchedulerLock sl;

            /* Try to find the page in the partially used list. */
            auto it = m_partially_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize)));
            if (it == m_partially_used_tlp_tree.end()) {
                /* If we don't find it, it has to be in the fully used list. */
                it = m_fully_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize)));
                R_UNLESS(it != m_fully_used_tlp_tree.end(), svc::ResultInvalidAddress());

                /* Release the region. */
                it->Release(addr);

                /* Move the page out of the fully used list. */
                KThreadLocalPage *tlp = std::addressof(*it);
                m_fully_used_tlp_tree.erase(it);
                if (tlp->IsAllFree()) {
                    page_to_free = tlp;
                } else {
                    m_partially_used_tlp_tree.insert(*tlp);
                }
            } else {
                /* Release the region. */
                it->Release(addr);

                /* Handle the all-free case. */
                KThreadLocalPage *tlp = std::addressof(*it);
                if (tlp->IsAllFree()) {
                    m_partially_used_tlp_tree.erase(it);
                    page_to_free = tlp;
                }
            }
        }

        /* If we should free the page it was in, do so. */
        if (page_to_free != nullptr) {
            page_to_free->Finalize();

            KThreadLocalPage::Free(page_to_free);
        }

        return ResultSuccess();
    }

    void *KProcess::GetThreadLocalRegionPointer(KProcessAddress addr) {
        KThreadLocalPage *tlp = nullptr;
        {
            KScopedSchedulerLock sl;
            if (auto it = m_partially_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize))); it != m_partially_used_tlp_tree.end()) {
                tlp = std::addressof(*it);
            } else if (auto it = m_fully_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize))); it != m_fully_used_tlp_tree.end()) {
                tlp = std::addressof(*it);
            } else {
                return nullptr;
            }
        }
        return static_cast<u8 *>(tlp->GetPointer()) + (GetInteger(addr) & (PageSize - 1));
    }

    bool KProcess::ReserveResource(ams::svc::LimitableResource which, s64 value) {
        if (KResourceLimit *rl = this->GetResourceLimit(); rl != nullptr) {
            return rl->Reserve(which, value);
        } else {
            return true;
        }
    }

    bool KProcess::ReserveResource(ams::svc::LimitableResource which, s64 value, s64 timeout) {
        if (KResourceLimit *rl = this->GetResourceLimit(); rl != nullptr) {
            return rl->Reserve(which, value, timeout);
        } else {
            return true;
        }
    }

    void KProcess::ReleaseResource(ams::svc::LimitableResource which, s64 value) {
        if (KResourceLimit *rl = this->GetResourceLimit(); rl != nullptr) {
            rl->Release(which, value);
        }
    }

    void KProcess::ReleaseResource(ams::svc::LimitableResource which, s64 value, s64 hint) {
        if (KResourceLimit *rl = this->GetResourceLimit(); rl != nullptr) {
            rl->Release(which, value, hint);
        }
    }

    void KProcess::IncrementThreadCount() {
        MESOSPHERE_ASSERT(m_num_threads >= 0);
        ++m_num_created_threads;

        if (const auto count = ++m_num_threads; count > m_peak_num_threads) {
            m_peak_num_threads = count;
        }
    }

    void KProcess::DecrementThreadCount() {
        MESOSPHERE_ASSERT(m_num_threads > 0);

        if (const auto count = --m_num_threads; count == 0) {
            this->Terminate();
        }
    }

    bool KProcess::EnterUserException() {
        /* Get the current thread. */
        KThread *cur_thread = GetCurrentThreadPointer();
        MESOSPHERE_ASSERT(this == cur_thread->GetOwnerProcess());

        /* Try to claim the exception thread. */
        if (m_exception_thread != cur_thread) {
            const uintptr_t address_key = reinterpret_cast<uintptr_t>(std::addressof(m_exception_thread));
            while (true) {
                {
                    KScopedSchedulerLock sl;

                    /* If the thread is terminating, it can't enter. */
                    if (cur_thread->IsTerminationRequested()) {
                        return false;
                    }

                    /* If we have no exception thread, we succeeded. */
                    if (m_exception_thread == nullptr) {
                        m_exception_thread = cur_thread;
                        return true;
                    }

                    /* Otherwise, wait for us to not have an exception thread. */
                    cur_thread->SetAddressKey(address_key);
                    m_exception_thread->AddWaiter(cur_thread);
                    if (cur_thread->GetState() == KThread::ThreadState_Runnable) {
                        cur_thread->SetState(KThread::ThreadState_Waiting);
                    } else {
                        KScheduler::SetSchedulerUpdateNeeded();
                    }
                }
                /* Remove the thread as a waiter from the lock owner. */
                {
                    KScopedSchedulerLock sl;
                    KThread *owner_thread = cur_thread->GetLockOwner();
                    if (owner_thread != nullptr) {
                        owner_thread->RemoveWaiter(cur_thread);
                        KScheduler::SetSchedulerUpdateNeeded();
                    }
                }
            }
        } else {
            return false;
        }
    }

    bool KProcess::LeaveUserException() {
        return this->ReleaseUserException(GetCurrentThreadPointer());
    }

    bool KProcess::ReleaseUserException(KThread *thread) {
        KScopedSchedulerLock sl;

        if (m_exception_thread == thread) {
            m_exception_thread = nullptr;

            /* Remove waiter thread. */
            s32 num_waiters;
            KThread *next = thread->RemoveWaiterByKey(std::addressof(num_waiters), reinterpret_cast<uintptr_t>(std::addressof(m_exception_thread)));
            if (next != nullptr) {
                if (next->GetState() == KThread::ThreadState_Waiting) {
                    next->SetState(KThread::ThreadState_Runnable);
                } else {
                    KScheduler::SetSchedulerUpdateNeeded();
                }
            }

            return true;
        } else {
            return false;
        }
    }

    void KProcess::RegisterThread(KThread *thread) {
        KScopedLightLock lk(m_list_lock);

        m_thread_list.push_back(*thread);
    }

    void KProcess::UnregisterThread(KThread *thread) {
        KScopedLightLock lk(m_list_lock);

        m_thread_list.erase(m_thread_list.iterator_to(*thread));
    }

    size_t KProcess::GetUsedUserPhysicalMemorySize() const {
        const size_t norm_size  = m_page_table.GetNormalMemorySize();
        const size_t other_size = m_code_size + m_main_thread_stack_size;
        const size_t sec_size   = KSystemControl::CalculateRequiredSecureMemorySize(m_system_resource_num_pages * PageSize, m_memory_pool);

        return norm_size + other_size + sec_size;
    }

    size_t KProcess::GetTotalUserPhysicalMemorySize() const {
        /* Get the amount of free and used size. */
        const size_t free_size = m_resource_limit->GetFreeValue(ams::svc::LimitableResource_PhysicalMemoryMax);
        const size_t used_size = this->GetUsedNonSystemUserPhysicalMemorySize();
        const size_t max_size  = m_max_process_memory;

        if (used_size + free_size > max_size) {
            return max_size;
        } else {
            return free_size + used_size;
        }
    }

    size_t KProcess::GetUsedNonSystemUserPhysicalMemorySize() const {
        const size_t norm_size  = m_page_table.GetNormalMemorySize();
        const size_t other_size = m_code_size + m_main_thread_stack_size;

        return norm_size + other_size;
    }

    size_t KProcess::GetTotalNonSystemUserPhysicalMemorySize() const {
        /* Get the amount of free and used size. */
        const size_t free_size = m_resource_limit->GetFreeValue(ams::svc::LimitableResource_PhysicalMemoryMax);
        const size_t used_size = this->GetUsedUserPhysicalMemorySize();
        const size_t sec_size  = KSystemControl::CalculateRequiredSecureMemorySize(m_system_resource_num_pages * PageSize, m_memory_pool);
        const size_t max_size  = m_max_process_memory;

        if (used_size + free_size > max_size) {
            return max_size - sec_size;
        } else {
            return free_size + used_size - sec_size;
        }
    }

    Result KProcess::Run(s32 priority, size_t stack_size) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock ourselves, to prevent concurrent access. */
        KScopedLightLock lk(m_state_lock);

        /* Validate that we're in a state where we can initialize. */
        const auto state = m_state;
        R_UNLESS(state == State_Created || state == State_CreatedAttached, svc::ResultInvalidState());

        /* Place a tentative reservation of a thread for this process. */
        KScopedResourceReservation thread_reservation(this, ams::svc::LimitableResource_ThreadCountMax);
        R_UNLESS(thread_reservation.Succeeded(), svc::ResultLimitReached());

        /* Ensure that we haven't already allocated stack. */
        MESOSPHERE_ABORT_UNLESS(m_main_thread_stack_size == 0);

        /* Ensure that we're allocating a valid stack. */
        stack_size = util::AlignUp(stack_size, PageSize);
        R_UNLESS(stack_size + m_code_size <= m_max_process_memory, svc::ResultOutOfMemory());
        R_UNLESS(stack_size + m_code_size >= m_code_size,          svc::ResultOutOfMemory());

        /* Place a tentative reservation of memory for our new stack. */
        KScopedResourceReservation mem_reservation(this, ams::svc::LimitableResource_PhysicalMemoryMax, stack_size);
        R_UNLESS(mem_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate and map our stack. */
        KProcessAddress stack_top = Null<KProcessAddress>;
        if (stack_size) {
            KProcessAddress stack_bottom;
            R_TRY(m_page_table.MapPages(std::addressof(stack_bottom), stack_size / PageSize, KMemoryState_Stack, KMemoryPermission_UserReadWrite));

            stack_top = stack_bottom + stack_size;
            m_main_thread_stack_size = stack_size;
        }

        /* Ensure our stack is safe to clean up on exit. */
        auto stack_guard = SCOPE_GUARD {
            if (m_main_thread_stack_size) {
                MESOSPHERE_R_ABORT_UNLESS(m_page_table.UnmapPages(stack_top - m_main_thread_stack_size, m_main_thread_stack_size / PageSize, KMemoryState_Stack));
                m_main_thread_stack_size = 0;
            }
        };

        /* Set our maximum heap size. */
        R_TRY(m_page_table.SetMaxHeapSize(m_max_process_memory - (m_main_thread_stack_size + m_code_size)));

        /* Initialize our handle table. */
        R_TRY(m_handle_table.Initialize(m_capabilities.GetHandleTableSize()));
        auto ht_guard = SCOPE_GUARD { m_handle_table.Finalize(); };

        /* Create a new thread for the process. */
        KThread *main_thread = KThread::Create();
        R_UNLESS(main_thread != nullptr, svc::ResultOutOfResource());
        auto thread_guard = SCOPE_GUARD { main_thread->Close(); };

        /* Initialize the thread. */
        R_TRY(KThread::InitializeUserThread(main_thread, reinterpret_cast<KThreadFunction>(GetVoidPointer(this->GetEntryPoint())), 0, stack_top, priority, m_ideal_core_id, this));

        /* Register the thread, and commit our reservation. */
        KThread::Register(main_thread);
        thread_reservation.Commit();

        /* Add the thread to our handle table. */
        ams::svc::Handle thread_handle;
        R_TRY(m_handle_table.Add(std::addressof(thread_handle), main_thread));

        /* Set the thread arguments. */
        main_thread->GetContext().SetArguments(0, thread_handle);

        /* Update our state. */
        this->ChangeState((state == State_Created) ? State_Running : State_RunningAttached);
        auto state_guard = SCOPE_GUARD { this->ChangeState(state); };

        /* Run our thread. */
        R_TRY(main_thread->Run());

        /* We succeeded! Cancel our guards. */
        state_guard.Cancel();
        thread_guard.Cancel();
        ht_guard.Cancel();
        stack_guard.Cancel();
        mem_reservation.Commit();

        /* Note for debug that we're running a new process. */
        MESOSPHERE_LOG("KProcess::Run() pid=%ld name=%-12s thread=%ld affinity=0x%lx ideal_core=%d active_core=%d\n", m_process_id, m_name, main_thread->GetId(), main_thread->GetVirtualAffinityMask(), main_thread->GetIdealVirtualCore(), main_thread->GetActiveCore());

        return ResultSuccess();
    }

    Result KProcess::Reset() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the process and the scheduler. */
        KScopedLightLock lk(m_state_lock);
        KScopedSchedulerLock sl;

        /* Validate that we're in a state that we can reset. */
        R_UNLESS(m_state != State_Terminated, svc::ResultInvalidState());
        R_UNLESS(m_is_signaled,               svc::ResultInvalidState());

        /* Clear signaled. */
        m_is_signaled = false;
        return ResultSuccess();
    }

    Result KProcess::SetActivity(ams::svc::ProcessActivity activity) {
        /* Lock ourselves and the scheduler. */
        KScopedLightLock lk(m_state_lock);
        KScopedLightLock list_lk(m_list_lock);
        KScopedSchedulerLock sl;

        /* Validate our state. */
        R_UNLESS(m_state != State_Terminating, svc::ResultInvalidState());
        R_UNLESS(m_state != State_Terminated,  svc::ResultInvalidState());

        /* Either pause or resume. */
        if (activity == ams::svc::ProcessActivity_Paused) {
            /* Verify that we're not suspended. */
            R_UNLESS(!m_is_suspended, svc::ResultInvalidState());

            /* Suspend all threads. */
            auto end = this->GetThreadList().end();
            for (auto it = this->GetThreadList().begin(); it != end; ++it) {
                it->RequestSuspend(KThread::SuspendType_Process);
            }

            /* Set ourselves as suspended. */
            this->SetSuspended(true);
        } else {
            MESOSPHERE_ASSERT(activity == ams::svc::ProcessActivity_Runnable);

            /* Verify that we're suspended. */
            R_UNLESS(m_is_suspended, svc::ResultInvalidState());

            /* Resume all threads. */
            auto end = this->GetThreadList().end();
            for (auto it = this->GetThreadList().begin(); it != end; ++it) {
                it->Resume(KThread::SuspendType_Process);
            }

            /* Set ourselves as resumed. */
            this->SetSuspended(false);
        }

        return ResultSuccess();
    }

    void KProcess::PinCurrentThread() {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Get the current thread. */
        const s32 core_id   = GetCurrentCoreId();
        KThread *cur_thread = GetCurrentThreadPointer();

        /* Pin it. */
        this->PinThread(core_id, cur_thread);
        cur_thread->Pin();

        /* An update is needed. */
        KScheduler::SetSchedulerUpdateNeeded();
    }

    void KProcess::UnpinCurrentThread() {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Get the current thread. */
        const s32 core_id   = GetCurrentCoreId();
        KThread *cur_thread = GetCurrentThreadPointer();

        /* Unpin it. */
        cur_thread->Unpin();
        this->UnpinThread(core_id, cur_thread);

        /* An update is needed. */
        KScheduler::SetSchedulerUpdateNeeded();
    }

    Result KProcess::GetThreadList(s32 *out_num_threads, ams::kern::svc::KUserPointer<u64 *> out_thread_ids, s32 max_out_count) {
        /* Lock the list. */
        KScopedLightLock lk(m_list_lock);

        /* Iterate over the list. */
        s32 count = 0;
        auto end = this->GetThreadList().end();
        for (auto it = this->GetThreadList().begin(); it != end; ++it) {
            /* If we're within array bounds, write the id. */
            if (count < max_out_count) {
                /* Get the thread id. */
                KThread *thread = std::addressof(*it);
                const u64 id = thread->GetId();

                /* Copy the id to userland. */
                R_TRY(out_thread_ids.CopyArrayElementFrom(std::addressof(id), count));
            }

            /* Increment the count. */
            ++count;
        }

        /* We successfully iterated the list. */
        *out_num_threads = count;
        return ResultSuccess();
    }

    KProcess::State KProcess::SetDebugObject(void *debug_object) {
        /* Attaching should only happen to non-null objects while the scheduler is locked. */
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(debug_object != nullptr);

        /* Cache our state to return it to the debug object. */
        const auto old_state = m_state;

        /* Set the object. */
        m_attached_object = debug_object;

        /* Check that our state is valid for attach. */
        MESOSPHERE_ASSERT(m_state == State_Created || m_state == State_Running || m_state == State_Crashed);

        /* Update our state. */
        if (m_state != State_DebugBreak) {
            if (m_state == State_Created) {
                this->ChangeState(State_CreatedAttached);
            } else {
                this->ChangeState(State_DebugBreak);
            }
        }

        return old_state;
    }

    void KProcess::ClearDebugObject(KProcess::State old_state) {
        /* Detaching from process should only happen while the scheduler is locked. */
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Clear the attached object. */
        m_attached_object = nullptr;

        /* Validate that the process is in an attached state. */
        MESOSPHERE_ASSERT(m_state == State_CreatedAttached || m_state == State_RunningAttached || m_state == State_DebugBreak || m_state == State_Terminating || m_state == State_Terminated);

        /* Change the state appropriately. */
        if (m_state == State_CreatedAttached) {
            this->ChangeState(State_Created);
        } else if (m_state == State_RunningAttached || m_state == State_DebugBreak) {
            /* Disallow transition back to created from running. */
            if (old_state == State_Created) {
                old_state = State_Running;
            }
            this->ChangeState(old_state);
        }
    }

    bool KProcess::EnterJitDebug(ams::svc::DebugEvent event, ams::svc::DebugException exception, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
        /* Check that we're the current process. */
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this == GetCurrentProcessPointer());

        /* If we aren't allowed to enter jit debug, don't. */
        if ((m_flags & ams::svc::CreateProcessFlag_EnableDebug) == 0) {
            return false;
        }

        /* We're the current process, so we should be some kind of running. */
        MESOSPHERE_ASSERT(m_state != State_Created);
        MESOSPHERE_ASSERT(m_state != State_CreatedAttached);
        MESOSPHERE_ASSERT(m_state != State_Terminated);

        /* Try to enter JIT debug. */
        while (true) {
            /* Lock ourselves and the scheduler. */
            KScopedLightLock lk(m_state_lock);
            KScopedLightLock list_lk(m_list_lock);
            KScopedSchedulerLock sl;

            /* If we're attached to a debugger, we're necessarily in debug. */
            if (this->IsAttachedToDebugger()) {
                return true;
            }

            /* If the current thread is terminating, we can't enter debug. */
            if (GetCurrentThread().IsTerminationRequested()) {
                return false;
            }

            /* We're not attached to debugger, so check that. */
            MESOSPHERE_ASSERT(m_state != State_RunningAttached);
            MESOSPHERE_ASSERT(m_state != State_DebugBreak);

            /* If we're terminating, we can't enter debug. */
            if (m_state != State_Running && m_state != State_Crashed) {
                MESOSPHERE_ASSERT(m_state == State_Terminating);
                return false;
            }

            /* If the current thread is suspended, retry. */
            if (GetCurrentThread().IsSuspended()) {
                continue;
            }

            /* Suspend all our threads. */
            {
                auto end = this->GetThreadList().end();
                for (auto it = this->GetThreadList().begin(); it != end; ++it) {
                    it->RequestSuspend(KThread::SuspendType_Debug);
                }
            }

            /* Change our state to crashed. */
            this->ChangeState(State_Crashed);

            /* Enter jit debug. */
            m_is_jit_debug             = true;
            m_jit_debug_event_type     = event;
            m_jit_debug_exception_type = exception;
            m_jit_debug_params[0]      = param1;
            m_jit_debug_params[1]      = param2;
            m_jit_debug_params[2]      = param3;
            m_jit_debug_params[3]      = param4;
            m_jit_debug_thread_id      = GetCurrentThread().GetId();

            /* Exit our retry loop. */
            break;
        }

        /* Check if our state indicates we're in jit debug. */
        {
            KScopedSchedulerLock sl;

            if (m_state == State_Running || m_state == State_RunningAttached || m_state == State_Crashed || m_state == State_DebugBreak) {
                return true;
            }
        }

        return false;
    }

    KEventInfo *KProcess::GetJitDebugInfo() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        if (m_is_jit_debug) {
            return KDebugBase::CreateDebugEvent(m_jit_debug_event_type, m_jit_debug_exception_type, m_jit_debug_params[0], m_jit_debug_params[1], m_jit_debug_params[2], m_jit_debug_params[3], m_jit_debug_thread_id);
        } else {
            return nullptr;
        }
    }

    void KProcess::ClearJitDebugInfo() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        m_is_jit_debug = false;
    }

    KProcess *KProcess::GetProcessFromId(u64 process_id) {
        /* Lock the list. */
        KProcess::ListAccessor accessor;
        const auto end = accessor.end();

        /* Iterate over the list. */
        for (auto it = accessor.begin(); it != end; ++it) {
            /* Get the process. */
            KProcess *process = static_cast<KProcess *>(std::addressof(*it));

            if (process->GetId() == process_id) {
                if (AMS_LIKELY(process->Open())) {
                    return process;
                }
            }
        }

        /* We failed to find the process. */
        return nullptr;
    }

    Result KProcess::GetProcessList(s32 *out_num_processes, ams::kern::svc::KUserPointer<u64 *> out_process_ids, s32 max_out_count) {
        /* Lock the list. */
        KProcess::ListAccessor accessor;
        const auto end = accessor.end();

        /* Iterate over the list. */
        s32 count = 0;
        for (auto it = accessor.begin(); it != end; ++it) {
            /* If we're within array bounds, write the id. */
            if (count < max_out_count) {
                /* Get the process id. */
                KProcess *process = static_cast<KProcess *>(std::addressof(*it));
                const u64 id = process->GetId();

                /* Copy the id to userland. */
                R_TRY(out_process_ids.CopyArrayElementFrom(std::addressof(id), count));
            }

            /* Increment the count. */
            ++count;
        }

        /* We successfully iterated the list. */
        *out_num_processes = count;
        return ResultSuccess();
    }

}
