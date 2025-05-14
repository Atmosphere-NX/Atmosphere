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

        constexpr u64 InitialProcessIdMin = 1;
        constexpr u64 InitialProcessIdMax = 0x50;

        constexpr u64 ProcessIdMin        = InitialProcessIdMax + 1;
        constexpr u64 ProcessIdMax        = std::numeric_limits<u64>::max();

        constinit util::Atomic<u64> g_initial_process_id = InitialProcessIdMin;
        constinit util::Atomic<u64> g_process_id         = ProcessIdMin;

        Result TerminateChildren(KProcess *process, const KThread *thread_to_not_terminate) {
            /* Request that all children threads terminate. */
            {
                KScopedLightLock proc_lk(process->GetListLock());
                KScopedSchedulerLock sl;

                if (thread_to_not_terminate != nullptr && process->GetPinnedThread(GetCurrentCoreId()) == thread_to_not_terminate) {
                    /* NOTE: Here Nintendo unpins the current thread instead of the thread_to_not_terminate. */
                    /* This is valid because the only caller which uses non-nullptr as argument uses GetCurrentThreadPointer(), */
                    /* but it's still notable because it seems incorrect at first glance. */
                    process->UnpinCurrentThread();
                }

                auto &thread_list = process->GetThreadList();
                for (auto it = thread_list.begin(); it != thread_list.end(); ++it) {
                    if (KThread *thread = std::addressof(*it); thread != thread_to_not_terminate) {
                        if (thread->GetState() != KThread::ThreadState_Terminated) {
                            thread->RequestTerminate();
                        }
                    }
                }
            }

            /* Wait for all children threads to terminate. */
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
                ON_SCOPE_EXIT { cur_child->Close(); };

                if (const Result terminate_result = cur_child->Terminate(); svc::ResultTerminationRequested::Includes(terminate_result)) {
                    R_THROW(terminate_result);
                }
            }

            R_SUCCEED();
        }

        class ThreadQueueImplForKProcessEnterUserException final : public KThreadQueue {
            private:
                KThread **m_exception_thread;
            public:
                constexpr ThreadQueueImplForKProcessEnterUserException(KThread **t) : KThreadQueue(), m_exception_thread(t) { /* ... */ }

                virtual void EndWait(KThread *waiting_thread, Result wait_result) override {
                    /* Set the exception thread. */
                    *m_exception_thread = waiting_thread;

                    /* Invoke the base end wait handler. */
                    KThreadQueue::EndWait(waiting_thread, wait_result);
                }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread as a waiter on its mutex owner. */
                    waiting_thread->GetLockOwner()->RemoveWaiter(waiting_thread);

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    void KProcess::Finalize() {
        /* Delete the process local region. */
        this->DeleteThreadLocalRegion(m_plr_address);

        /* Get the used memory size. */
        const size_t used_memory_size = this->GetUsedNonSystemUserPhysicalMemorySize();

        /* Finalize the page table. */
        m_page_table.Finalize();

        /* Finish using our system resource. */
        {
            if (m_system_resource->IsSecureResource()) {
                /* Finalize optimized memory. If memory wasn't optimized, this is a no-op. */
                Kernel::GetMemoryManager().FinalizeOptimizedMemory(this->GetId(), m_memory_pool);
            }

            m_system_resource->Close();
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

        /* Close all references to our io regions. */
        {
            auto it = m_io_region_list.begin();
            while (it != m_io_region_list.end()) {
                KIoRegion *io_region = std::addressof(*it);
                it = m_io_region_list.erase(it);

                io_region->Close();
            }
        }

        /* Our thread local page list must be empty at this point. */
        MESOSPHERE_ABORT_UNLESS(m_partially_used_tlp_tree.empty());
        MESOSPHERE_ABORT_UNLESS(m_fully_used_tlp_tree.empty());

        /* Release memory to the resource limit. */
        if (m_resource_limit != nullptr) {
            MESOSPHERE_ABORT_UNLESS(used_memory_size >= m_memory_release_hint);
            m_resource_limit->Release(ams::svc::LimitableResource_PhysicalMemoryMax, used_memory_size, used_memory_size - m_memory_release_hint);
            m_resource_limit->Close();
        }

        /* Log that we finalized for debug. */
        MESOSPHERE_LOG("KProcess::Finalize() pid=%ld name=%-12s\n", m_process_id, m_name);

        /* Perform inherited finalization. */
        KSynchronizationObject::Finalize();
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
        m_used_kernel_memory_size   = 0;
        m_ideal_core_id             = 0;
        m_flags                     = params.flags;
        m_version                   = params.version;
        m_program_id                = params.program_id;
        m_code_address              = params.code_address;
        m_code_size                 = params.code_num_pages * PageSize;
        m_is_application            = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
        m_is_jit_debug              = false;

        #if defined(MESOSPHERE_ENABLE_PROCESS_CREATION_TIME)
        m_creation_time             = KHardwareTimer::GetTick();
        #endif

        /* Set thread fields. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            m_running_threads[i]              = nullptr;
            m_pinned_threads[i]               = nullptr;
            m_running_thread_idle_counts[i]   = 0;
            m_running_thread_switch_counts[i] = 0;
        }

        /* Set max memory. */
        m_max_process_memory = KAddressSpaceInfo::GetAddressSpaceSize(static_cast<ams::svc::CreateProcessFlag>(m_flags), KAddressSpaceInfo::Type_Heap);

        /* Generate random entropy. */
        KSystemControl::GenerateRandom(m_entropy, util::size(m_entropy));

        /* Clear remaining fields. */
        m_num_running_threads         = 0;
        m_num_process_switches        = 0;
        m_num_thread_switches         = 0;
        m_num_fpu_switches            = 0;
        m_num_supervisor_calls        = 0;
        m_num_ipc_messages            = 0;

        m_is_signaled                 = false;
        m_attached_object             = nullptr;
        m_exception_thread            = nullptr;
        m_is_suspended                = false;
        m_memory_release_hint         = 0;
        m_schedule_count              = 0;
        m_is_handle_table_initialized = false;

        /* We're initialized! */
        m_is_initialized = true;

        R_SUCCEED();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool, bool immortal) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);
        MESOSPHERE_ABORT_UNLESS((params.code_num_pages * PageSize) / PageSize == static_cast<size_t>(params.code_num_pages));

        /* Set members. */
        m_memory_pool                            = pool;
        m_resource_limit                         = res_limit;
        m_is_default_application_system_resource = false;
        m_is_immortal                            = immortal;

        /* Setup our system resource. */
        if (const size_t system_resource_num_pages = params.system_resource_num_pages; system_resource_num_pages != 0) {
            /* Create a secure system resource. */
            KSecureSystemResource *secure_resource = KSecureSystemResource::Create();
            R_UNLESS(secure_resource != nullptr, svc::ResultOutOfResource());

            ON_RESULT_FAILURE { secure_resource->Close(); };

            /* Initialize the secure resource. */
            R_TRY(secure_resource->Initialize(system_resource_num_pages * PageSize, m_resource_limit, m_memory_pool));

            /* Set our system resource. */
            m_system_resource = secure_resource;
        } else {
            /* Use the system-wide system resource. */
            const bool is_app = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
            m_system_resource = std::addressof(is_app ? Kernel::GetApplicationSystemResource() : Kernel::GetSystemSystemResource());

            m_is_default_application_system_resource = is_app;

            /* Open reference to the system resource. */
            m_system_resource->Open();
        }

        /* Ensure we clean up our secure resource, if we fail. */
        ON_RESULT_FAILURE { m_system_resource->Close(); };

        /* Setup page table. */
        {
            const bool from_back = (params.flags & ams::svc::CreateProcessFlag_EnableAslr) == 0;
            R_TRY(m_page_table.Initialize(static_cast<ams::svc::CreateProcessFlag>(params.flags), from_back, pool, params.code_address, params.code_num_pages * PageSize, m_system_resource, res_limit, this->GetSlabIndex()));
        }
        ON_RESULT_FAILURE_2 { m_page_table.Finalize(); };

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
        R_SUCCEED();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);

        /* Set pool and resource limit. */
        m_memory_pool                            = pool;
        m_resource_limit                         = res_limit;
        m_is_default_application_system_resource = false;
        m_is_immortal                            = false;

        /* Get the memory sizes. */
        const size_t code_num_pages            = params.code_num_pages;
        const size_t system_resource_num_pages = params.system_resource_num_pages;
        const size_t code_size                 = code_num_pages * PageSize;
        const size_t system_resource_size      = system_resource_num_pages * PageSize;

        /* Reserve memory for our code resource. */
        KScopedResourceReservation memory_reservation(this, ams::svc::LimitableResource_PhysicalMemoryMax, code_size);
        R_UNLESS(memory_reservation.Succeeded(), svc::ResultLimitReached());

        /* Setup our system resource. */
        if (system_resource_num_pages != 0) {
            /* Create a secure system resource. */
            KSecureSystemResource *secure_resource = KSecureSystemResource::Create();
            R_UNLESS(secure_resource != nullptr, svc::ResultOutOfResource());

            ON_RESULT_FAILURE { secure_resource->Close(); };

            /* Initialize the secure resource. */
            R_TRY(secure_resource->Initialize(system_resource_size, m_resource_limit, m_memory_pool));

            /* Set our system resource. */
            m_system_resource = secure_resource;

        } else {
            /* Use the system-wide system resource. */
            const bool is_app = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
            m_system_resource = std::addressof(is_app ? Kernel::GetApplicationSystemResource() : Kernel::GetSystemSystemResource());

            m_is_default_application_system_resource = is_app;

            /* Open reference to the system resource. */
            m_system_resource->Open();
        }

        /* Ensure we clean up our secure resource, if we fail. */
        ON_RESULT_FAILURE { m_system_resource->Close(); };

        /* Setup page table. */
        {
            const bool from_back = (params.flags & ams::svc::CreateProcessFlag_EnableAslr) == 0;
            R_TRY(m_page_table.Initialize(static_cast<ams::svc::CreateProcessFlag>(params.flags), from_back, pool, params.code_address, code_size, m_system_resource, res_limit, this->GetSlabIndex()));
        }
        ON_RESULT_FAILURE_2 { m_page_table.Finalize(); };

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
        if (m_system_resource->IsSecureResource() && (params.flags & ams::svc::CreateProcessFlag_OptimizeMemoryAllocation) != 0) {
            R_TRY(Kernel::GetMemoryManager().InitializeOptimizedMemory(m_process_id, pool));
        }

        /* Initialize the rest of the process. */
        R_TRY(this->Initialize(params));

        /* Open a reference to the resource limit. */
        m_resource_limit->Open();

        /* We succeeded, so commit our memory reservation. */
        memory_reservation.Commit();
        R_SUCCEED();
    }

    void KProcess::DoWorkerTaskImpl() {
        /* Terminate child threads. */
        TerminateChildren(this, nullptr);

        /* Finalize the handle table, if we're not immortal. */
        if (!m_is_immortal && m_is_handle_table_initialized) {
            this->FinalizeHandleTable();
        }

        /* Call the debug callback. */
        KDebug::OnExitProcess(this);

        /* Finish termination. */
        this->FinishTermination();
    }

    Result KProcess::StartTermination() {
        /* Finalize the handle table when we're done, if the process isn't immortal. */
        ON_SCOPE_EXIT {
            if (!m_is_immortal) {
                this->FinalizeHandleTable();
            }
        };

        /* Terminate child threads other than the current one. */
        R_RETURN(TerminateChildren(this, GetCurrentThreadPointer()));
    }

    void KProcess::FinishTermination() {
        /* Only allow termination to occur if the process isn't immortal. */
        if (!m_is_immortal) {
            /* Release resource limit hint. */
            if (m_resource_limit != nullptr) {
                m_memory_release_hint = this->GetUsedNonSystemUserPhysicalMemorySize();
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
    }

    void KProcess::Exit() {
        MESOSPHERE_ASSERT_THIS();

        /* Determine whether we need to start terminating. */
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
            KWorkerTaskManager::AddTask(KWorkerTaskManager::WorkerType_ExitProcess, this);
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
            if (R_SUCCEEDED(this->StartTermination())) {
                /* Note for debug that we're terminating the process. */
                MESOSPHERE_LOG("KProcess::Terminate() OK pid=%ld name=%-12s\n", m_process_id, m_name);

                /* Call the debug callback. */
                KDebug::OnTerminateProcess(this);

                /* Finish termination. */
                this->FinishTermination();
            } else {
                /* Note for debug that we're terminating the process. */
                MESOSPHERE_LOG("KProcess::Terminate() FAIL pid=%ld name=%-12s\n", m_process_id, m_name);

                /* Register the process as a work task. */
                KWorkerTaskManager::AddTask(KWorkerTaskManager::WorkerType_ExitProcess, this);
            }
        }

        R_SUCCEED();
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

        R_SUCCEED();
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

    void KProcess::AddIoRegion(KIoRegion *io_region) {
        /* Lock ourselves, to prevent concurrent access. */
        KScopedLightLock lk(m_state_lock);

        /* Open a reference to the region. */
        io_region->Open();

        /* Add the region to our list. */
        m_io_region_list.push_back(*io_region);

    }

    void KProcess::RemoveIoRegion(KIoRegion *io_region) {
        /* Remove the region from our list. */
        {
            /* Lock ourselves, to prevent concurrent access. */
            KScopedLightLock lk(m_state_lock);

            /* Remove the region from our list. */
            m_io_region_list.erase(m_io_region_list.iterator_to(*io_region));
        }

        /* Close our reference to the io region. */
        io_region->Close();
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
                R_SUCCEED();
            }
        }

        /* Allocate a new page. */
        tlp = KThreadLocalPage::Allocate();
        R_UNLESS(tlp != nullptr, svc::ResultOutOfMemory());
        ON_RESULT_FAILURE { KThreadLocalPage::Free(tlp); };

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
        *out = tlr;
        R_SUCCEED();
    }

    Result KProcess::DeleteThreadLocalRegion(KProcessAddress addr) {
        KThreadLocalPage *page_to_free = nullptr;

        /* Release the region. */
        {
            KScopedSchedulerLock sl;

            /* Try to find the page in the partially used list. */
            auto it = m_partially_used_tlp_tree.find_key(util::AlignDown(GetInteger(addr), PageSize));
            if (it == m_partially_used_tlp_tree.end()) {
                /* If we don't find it, it has to be in the fully used list. */
                it = m_fully_used_tlp_tree.find_key(util::AlignDown(GetInteger(addr), PageSize));
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

        R_SUCCEED();
    }

    void *KProcess::GetThreadLocalRegionPointer(KProcessAddress addr) {
        KThreadLocalPage *tlp = nullptr;
        {
            KScopedSchedulerLock sl;
            if (auto it = m_partially_used_tlp_tree.find_key(util::AlignDown(GetInteger(addr), PageSize)); it != m_partially_used_tlp_tree.end()) {
                tlp = std::addressof(*it);
            } else if (auto it = m_fully_used_tlp_tree.find_key(util::AlignDown(GetInteger(addr), PageSize)); it != m_fully_used_tlp_tree.end()) {
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

    void KProcess::IncrementRunningThreadCount() {
        MESOSPHERE_ASSERT(m_num_running_threads.Load() >= 0);

        ++m_num_running_threads;
    }

    void KProcess::DecrementRunningThreadCount() {
        MESOSPHERE_ASSERT(m_num_running_threads.Load() > 0);

        if (const auto prev = m_num_running_threads--; prev == 1) {
            this->Terminate();
        }
    }

    bool KProcess::EnterUserException() {
        /* Get the current thread. */
        KThread *cur_thread = GetCurrentThreadPointer();
        MESOSPHERE_ASSERT(this == cur_thread->GetOwnerProcess());

        /* Check that we haven't already claimed the exception thread. */
        if (m_exception_thread == cur_thread) {
            return false;
        }

        /* Create the wait queue we'll be using. */
        ThreadQueueImplForKProcessEnterUserException wait_queue(std::addressof(m_exception_thread));

        /* Claim the exception thread. */
        {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Check that we're not terminating. */
            if (cur_thread->IsTerminationRequested()) {
                return false;
            }

            /* If we don't have an exception thread, we can just claim it directly. */
            if (m_exception_thread == nullptr) {
                m_exception_thread = cur_thread;
                KScheduler::SetSchedulerUpdateNeeded();
                return true;
            }

            /* Otherwise, we need to wait until we don't have an exception thread. */

            /* Add the current thread as a waiter on the current exception thread. */
            cur_thread->SetAddressKey(reinterpret_cast<uintptr_t>(std::addressof(m_exception_thread)) | 1);
            m_exception_thread->AddWaiter(cur_thread);

            /* Wait to claim the exception thread. */
            cur_thread->BeginWait(std::addressof(wait_queue));
        }

        /* If our wait didn't end due to thread termination, we succeeded. */
        return !svc::ResultTerminationRequested::Includes(cur_thread->GetWaitResult());
    }

    bool KProcess::LeaveUserException() {
        return this->ReleaseUserException(GetCurrentThreadPointer());
    }

    bool KProcess::ReleaseUserException(KThread *thread) {
        KScopedSchedulerLock sl;

        if (m_exception_thread == thread) {
            m_exception_thread = nullptr;

            /* Remove waiter thread. */
            bool has_waiters;
            if (KThread *next = thread->RemoveWaiterByKey(std::addressof(has_waiters), reinterpret_cast<uintptr_t>(std::addressof(m_exception_thread)) | 1); next != nullptr) {
                next->EndWait(ResultSuccess());
            }

            KScheduler::SetSchedulerUpdateNeeded();

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
        const size_t sec_size   = this->GetRequiredSecureMemorySizeNonDefault();

        return norm_size + other_size + sec_size;
    }

    size_t KProcess::GetTotalUserPhysicalMemorySize() const {
        /* Get the amount of free and used size. */
        const size_t free_size = m_resource_limit->GetFreeValue(ams::svc::LimitableResource_PhysicalMemoryMax);
        const size_t max_size  = m_max_process_memory;

        /* Determine used size. */
        /* NOTE: This does *not* check this->IsDefaultApplicationSystemResource(), unlike GetUsedUserPhysicalMemorySize(). */
        const size_t norm_size  = m_page_table.GetNormalMemorySize();
        const size_t other_size = m_code_size + m_main_thread_stack_size;
        const size_t sec_size   = this->GetRequiredSecureMemorySize();
        const size_t used_size  = norm_size + other_size + sec_size;

        /* NOTE: These function calls will recalculate, introducing a race...it is unclear why Nintendo does it this way. */
        if (used_size + free_size > max_size) {
            return max_size;
        } else {
            return free_size + this->GetUsedUserPhysicalMemorySize();
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
        const size_t max_size  = m_max_process_memory;

        /* Determine used size. */
        /* NOTE: This does *not* check this->IsDefaultApplicationSystemResource(), unlike GetUsedUserPhysicalMemorySize(). */
        const size_t norm_size  = m_page_table.GetNormalMemorySize();
        const size_t other_size = m_code_size + m_main_thread_stack_size;
        const size_t sec_size   = this->GetRequiredSecureMemorySize();
        const size_t used_size  = norm_size + other_size + sec_size;

        /* NOTE: These function calls will recalculate, introducing a race...it is unclear why Nintendo does it this way. */
        if (used_size + free_size > max_size) {
            return max_size - this->GetRequiredSecureMemorySizeNonDefault();
        } else {
            return free_size + this->GetUsedNonSystemUserPhysicalMemorySize();
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
        ON_RESULT_FAILURE {
            if (m_main_thread_stack_size) {
                MESOSPHERE_R_ABORT_UNLESS(m_page_table.UnmapPages(stack_top - m_main_thread_stack_size, m_main_thread_stack_size / PageSize, KMemoryState_Stack));
                m_main_thread_stack_size = 0;
            }
        };

        /* Set our maximum heap size. */
        R_TRY(m_page_table.SetMaxHeapSize(m_max_process_memory - (m_main_thread_stack_size + m_code_size)));

        /* Initialize our handle table. */
        R_TRY(this->InitializeHandleTable(m_capabilities.GetHandleTableSize()));
        ON_RESULT_FAILURE_2 { this->FinalizeHandleTable(); };

        /* Create a new thread for the process. */
        KThread *main_thread = KThread::Create();
        R_UNLESS(main_thread != nullptr, svc::ResultOutOfResource());
        ON_SCOPE_EXIT { main_thread->Close(); };

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
        ON_RESULT_FAILURE_2 { this->ChangeState(state); };

        /* Run our thread. */
        R_TRY(main_thread->Run());

        /* Open a reference to represent that we're running. */
        this->Open();

        /* We succeeded! Commit our memory reservation. */
        mem_reservation.Commit();

        /* Note for debug that we're running a new process. */
        MESOSPHERE_LOG("KProcess::Run() pid=%ld name=%-12s thread=%ld affinity=0x%lx ideal_core=%d active_core=%d\n", m_process_id, m_name, main_thread->GetId(), main_thread->GetVirtualAffinityMask(), main_thread->GetIdealVirtualCore(), main_thread->GetActiveCore());

        R_SUCCEED();
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
        R_SUCCEED();
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

        R_SUCCEED();
    }

    void KProcess::PinCurrentThread() {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Get the current thread. */
        const s32 core_id   = GetCurrentCoreId();
        KThread *cur_thread = GetCurrentThreadPointer();

        /* If the thread isn't terminated, pin it. */
        if (!cur_thread->IsTerminationRequested()) {
            /* Pin it. */
            this->PinThread(core_id, cur_thread);
            cur_thread->Pin();

            /* An update is needed. */
            KScheduler::SetSchedulerUpdateNeeded();
        }
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

    void KProcess::UnpinThread(KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Get the thread's core id. */
        const auto core_id = thread->GetActiveCore();

        /* Unpin it. */
        this->UnpinThread(core_id, thread);
        thread->Unpin();

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
        R_SUCCEED();
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
            const uintptr_t params[5] = { m_jit_debug_exception_type, m_jit_debug_params[0], m_jit_debug_params[1], m_jit_debug_params[2], m_jit_debug_params[3] };
            return KDebugBase::CreateDebugEvent(m_jit_debug_event_type, m_jit_debug_thread_id, params, util::size(params));
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
        R_SUCCEED();
    }

}
