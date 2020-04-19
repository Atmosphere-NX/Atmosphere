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
        std::atomic<u64> g_initial_process_id = InitialProcessIdMin;

    }

    void KProcess::Finalize() {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params) {
        /* TODO: Validate intended kernel version. */
        /* How should we do this? */

        /* Create and clear the process local region. */
        R_TRY(this->CreateThreadLocalRegion(std::addressof(this->plr_address)));
        this->plr_heap_address = this->GetThreadLocalRegionPointer(this->plr_address);
        std::memset(this->plr_heap_address, 0, ams::svc::ThreadLocalRegionSize);

        /* Copy in the name from parameters. */
        static_assert(sizeof(params.name) < sizeof(this->name));
        std::memcpy(this->name, params.name, sizeof(params.name));
        this->name[sizeof(params.name)] = 0;

        /* Set misc fields. */
        this->state                     = State_Created;
        this->main_thread_stack_size    = 0;
        this->creation_time             = KHardwareTimer::GetTick();
        this->used_kernel_memory_size   = 0;
        this->ideal_core_id             = 0;
        this->flags                     = params.flags;
        this->version                   = params.version;
        this->program_id                = params.program_id;
        this->code_address              = params.code_address;
        this->code_size                 = params.code_num_pages * PageSize;
        this->is_application            = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
        this->is_jit_debug              = false;

        /* Set thread fields. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            this->running_threads[i]            = nullptr;
            this->running_thread_idle_counts[i] = 0;
            this->pinned_threads[i]             = nullptr;
        }

        /* Set max memory based on address space type. */
        switch ((params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask)) {
            case ams::svc::CreateProcessFlag_AddressSpace32Bit:
            case ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated:
            case ams::svc::CreateProcessFlag_AddressSpace64Bit:
                this->max_process_memory = this->page_table.GetHeapRegionSize();
                break;
            case ams::svc::CreateProcessFlag_AddressSpace32BitWithoutAlias:
                this->max_process_memory = this->page_table.GetHeapRegionSize() + this->page_table.GetAliasRegionSize();
                break;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }

        /* Generate random entropy. */
        KSystemControl::GenerateRandomBytes(this->entropy, sizeof(this->entropy));

        /* Clear remaining fields. */
        this->num_threads           = 0;
        this->peak_num_threads      = 0;
        this->num_created_threads   = 0;
        this->num_process_switches  = 0;
        this->num_thread_switches   = 0;
        this->num_fpu_switches      = 0;
        this->num_supervisor_calls  = 0;
        this->num_ipc_messages      = 0;

        this->is_signaled           = false;
        this->attached_object       = nullptr;
        this->exception_thread      = nullptr;
        this->is_suspended          = false;
        this->memory_release_hint   = 0;
        this->schedule_count        = 0;

        /* We're initialized! */
        this->is_initialized = true;

        return ResultSuccess();
    }

    Result KProcess::Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(res_limit != nullptr);
        MESOSPHERE_ABORT_UNLESS((params.code_num_pages * PageSize) / PageSize == params.code_num_pages);

        /* Set members. */
        this->memory_pool               = pool;
        this->resource_limit            = res_limit;
        this->system_resource_address   = Null<KVirtualAddress>;
        this->system_resource_num_pages = 0;

        /* Setup page table. */
        /* NOTE: Nintendo passes process ID despite not having set it yet. */
        /* This goes completely unused, but even so... */
        {
            const auto as_type       = static_cast<ams::svc::CreateProcessFlag>(params.flags & ams::svc::CreateProcessFlag_AddressSpaceMask);
            const bool enable_aslr   = (params.flags & ams::svc::CreateProcessFlag_EnableAslr);
            const bool is_app        = (params.flags & ams::svc::CreateProcessFlag_IsApplication);
            auto *mem_block_manager  = std::addressof(is_app ? Kernel::GetApplicationMemoryBlockManager() : Kernel::GetSystemMemoryBlockManager());
            auto *block_info_manager = std::addressof(Kernel::GetBlockInfoManager());
            auto *pt_manager         = std::addressof(Kernel::GetPageTableManager());
            R_TRY(this->page_table.Initialize(this->process_id, as_type, enable_aslr, !enable_aslr, pool, params.code_address, params.code_num_pages * PageSize, mem_block_manager, block_info_manager, pt_manager));
        }
        auto pt_guard = SCOPE_GUARD { this->page_table.Finalize(); };

        /* Ensure we can insert the code region. */
        R_UNLESS(this->page_table.CanContain(params.code_address, params.code_num_pages * PageSize, KMemoryState_Code), svc::ResultInvalidMemoryRegion());

        /* Map the code region. */
        R_TRY(this->page_table.MapPageGroup(params.code_address, pg, KMemoryState_Code, KMemoryPermission_KernelRead));

        /* Initialize capabilities. */
        R_TRY(this->capabilities.Initialize(caps, num_caps, std::addressof(this->page_table)));

        /* Initialize the process id. */
        this->process_id = g_initial_process_id++;
        MESOSPHERE_ABORT_UNLESS(InitialProcessIdMin <= this->process_id);
        MESOSPHERE_ABORT_UNLESS(this->process_id <= InitialProcessIdMax);

        /* Initialize the rest of the process. */
        R_TRY(this->Initialize(params));

        /* Open a reference to the resource limit. */
        this->resource_limit->Open();

        /* We succeeded! */
        pt_guard.Cancel();
        return ResultSuccess();
    }

    void KProcess::DoWorkerTask() {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KProcess::CreateThreadLocalRegion(KProcessAddress *out) {
        KThreadLocalPage *tlp = nullptr;
        KProcessAddress   tlr = Null<KProcessAddress>;

        /* See if we can get a region from a partially used TLP. */
        {
            KScopedSchedulerLock sl;

            if (auto it = this->partially_used_tlp_tree.begin(); it != partially_used_tlp_tree.end()) {
                tlr = it->Reserve();
                MESOSPHERE_ABORT_UNLESS(tlr != Null<KProcessAddress>);

                if (it->IsAllUsed()) {
                    tlp = std::addressof(*it);
                    this->partially_used_tlp_tree.erase(it);
                    this->fully_used_tlp_tree.insert(*tlp);
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
                this->fully_used_tlp_tree.insert(*tlp);
            } else {
                this->partially_used_tlp_tree.insert(*tlp);
            }
        }

        /* We succeeded! */
        tlp_guard.Cancel();
        *out = tlr;
        return ResultSuccess();
    }

    void *KProcess::GetThreadLocalRegionPointer(KProcessAddress addr) {
        KThreadLocalPage *tlp = nullptr;
        {
            KScopedSchedulerLock sl;
            if (auto it = this->partially_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize))); it != this->partially_used_tlp_tree.end()) {
                tlp = std::addressof(*it);
            } else if (auto it = this->fully_used_tlp_tree.find(KThreadLocalPage(util::AlignDown(GetInteger(addr), PageSize))); it != this->fully_used_tlp_tree.end()) {
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
        MESOSPHERE_ASSERT(this->num_threads >= 0);
        ++this->num_created_threads;

        if (const auto count = ++this->num_threads; count > this->peak_num_threads) {
            this->peak_num_threads = count;
        }
    }

    void KProcess::DecrementThreadCount() {
        MESOSPHERE_ASSERT(this->num_threads > 0);

        if (const auto count = --this->num_threads; count == 0) {
            MESOSPHERE_TODO("this->Terminate();");
        }
    }

    void KProcess::RegisterThread(KThread *thread) {
        KScopedLightLock lk(this->list_lock);

        this->thread_list.push_back(*thread);
    }

    void KProcess::UnregisterThread(KThread *thread) {
        KScopedLightLock lk(this->list_lock);

        this->thread_list.erase(this->thread_list.iterator_to(*thread));
    }

    Result KProcess::Run(s32 priority, size_t stack_size) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock ourselves, to prevent concurrent access. */
        KScopedLightLock lk(this->lock);

        /* Validate that we're in a state where we can initialize. */
        const auto state = this->state;
        R_UNLESS(state == State_Created || state == State_CreatedAttached, svc::ResultInvalidState());

        /* Place a tentative reservation of a thread for this process. */
        KScopedResourceReservation thread_reservation(this, ams::svc::LimitableResource_ThreadCountMax);
        R_UNLESS(thread_reservation.Succeeded(), svc::ResultLimitReached());

        /* Ensure that we haven't already allocated stack. */
        MESOSPHERE_ABORT_UNLESS(this->main_thread_stack_size == 0);

        /* Ensure that we're allocating a valid stack. */
        stack_size = util::AlignUp(stack_size, PageSize);
        R_UNLESS(stack_size + this->code_size <= this->max_process_memory, svc::ResultOutOfMemory());
        R_UNLESS(stack_size + this->code_size >= this->code_size,          svc::ResultOutOfMemory());

        /* Place a tentative reservation of memory for our new stack. */
        KScopedResourceReservation mem_reservation(this, ams::svc::LimitableResource_PhysicalMemoryMax);
        R_UNLESS(mem_reservation.Succeeded(), svc::ResultLimitReached());

        /* Allocate and map our stack. */
        KProcessAddress stack_top = Null<KProcessAddress>;
        if (stack_size) {
            KProcessAddress stack_bottom;
            R_TRY(this->page_table.MapPages(std::addressof(stack_bottom), stack_size / PageSize, KMemoryState_Stack, KMemoryPermission_UserReadWrite));

            stack_top = stack_bottom + stack_size;
            this->main_thread_stack_size = stack_size;
        }

        /* Ensure our stack is safe to clean up on exit. */
        auto stack_guard = SCOPE_GUARD {
            if (this->main_thread_stack_size) {
                MESOSPHERE_R_ABORT_UNLESS(this->page_table.UnmapPages(stack_top - this->main_thread_stack_size, this->main_thread_stack_size / PageSize, KMemoryState_Stack));
                this->main_thread_stack_size = 0;
            }
        };

        /* Set our maximum heap size. */
        R_TRY(this->page_table.SetMaxHeapSize(this->max_process_memory - (this->main_thread_stack_size + this->code_size)));

        /* Initialize our handle table. */
        R_TRY(this->handle_table.Initialize(this->capabilities.GetHandleTableSize()));
        auto ht_guard = SCOPE_GUARD { this->handle_table.Finalize(); };

        /* Create a new thread for the process. */
        KThread *main_thread = KThread::Create();
        R_UNLESS(main_thread != nullptr, svc::ResultOutOfResource());
        auto thread_guard = SCOPE_GUARD { main_thread->Close(); };

        /* Initialize the thread. */
        R_TRY(KThread::InitializeUserThread(main_thread, reinterpret_cast<KThreadFunction>(GetVoidPointer(this->GetEntryPoint())), 0, stack_top, priority, this->ideal_core_id, this));

        /* Register the thread, and commit our reservation. */
        KThread::Register(main_thread);
        thread_reservation.Commit();

        /* Add the thread to our handle table. */
        ams::svc::Handle thread_handle;
        R_TRY(this->handle_table.Add(std::addressof(thread_handle), main_thread));

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
        MESOSPHERE_LOG("KProcess::Run() pid=%ld name=%-12s thread=%ld affinity=0x%lx ideal_core=%d active_core=%d\n", this->process_id, this->name, main_thread->GetId(), main_thread->GetAffinityMask().GetAffinityMask(), main_thread->GetIdealCore(), main_thread->GetActiveCore());

        return ResultSuccess();
    }

    void KProcess::SetPreemptionState() {
        MESOSPHERE_UNIMPLEMENTED();
    }

}
