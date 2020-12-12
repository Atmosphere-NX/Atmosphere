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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_handle_table.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_thread_local_page.hpp>
#include <mesosphere/kern_k_shared_memory_info.hpp>
#include <mesosphere/kern_k_beta.hpp>
#include <mesosphere/kern_k_worker_task.hpp>
#include <mesosphere/kern_select_page_table.hpp>
#include <mesosphere/kern_k_condition_variable.hpp>
#include <mesosphere/kern_k_address_arbiter.hpp>
#include <mesosphere/kern_k_capabilities.hpp>
#include <mesosphere/kern_k_wait_object.hpp>
#include <mesosphere/kern_k_dynamic_slab_heap.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>

namespace ams::kern {

    class KProcess final : public KAutoObjectWithSlabHeapAndContainer<KProcess, KSynchronizationObject>, public KWorkerTask {
        MESOSPHERE_AUTOOBJECT_TRAITS(KProcess, KSynchronizationObject);
        public:
            enum State {
                State_Created         = ams::svc::ProcessState_Created,
                State_CreatedAttached = ams::svc::ProcessState_CreatedAttached,
                State_Running         = ams::svc::ProcessState_Running,
                State_Crashed         = ams::svc::ProcessState_Crashed,
                State_RunningAttached = ams::svc::ProcessState_RunningAttached,
                State_Terminating     = ams::svc::ProcessState_Terminating,
                State_Terminated      = ams::svc::ProcessState_Terminated,
                State_DebugBreak      = ams::svc::ProcessState_DebugBreak,
            };

            using ThreadList = util::IntrusiveListMemberTraits<&KThread::process_list_node>::ListType;

            static constexpr size_t AslrAlignment = KernelAslrAlignment;
        private:
            using SharedMemoryInfoList = util::IntrusiveListBaseTraits<KSharedMemoryInfo>::ListType;
            using BetaList = util::IntrusiveListMemberTraits<&KBeta::process_list_node>::ListType;
            using TLPTree = util::IntrusiveRedBlackTreeBaseTraits<KThreadLocalPage>::TreeType<KThreadLocalPage>;
            using TLPIterator = TLPTree::iterator;
        private:
            KProcessPageTable           page_table{};
            std::atomic<size_t>         used_kernel_memory_size{};
            TLPTree                     fully_used_tlp_tree{};
            TLPTree                     partially_used_tlp_tree{};
            s32                         ideal_core_id{};
            void                       *attached_object{};
            KResourceLimit             *resource_limit{};
            KVirtualAddress             system_resource_address{};
            size_t                      system_resource_num_pages{};
            size_t                      memory_release_hint{};
            State                       state{};
            KLightLock                  state_lock{};
            KLightLock                  list_lock{};
            KConditionVariable          cond_var{};
            KAddressArbiter             address_arbiter{};
            u64                         entropy[4]{};
            bool                        is_signaled{};
            bool                        is_initialized{};
            bool                        is_application{};
            char                        name[13]{};
            std::atomic<u16>            num_threads{};
            u16                         peak_num_threads{};
            u32                         flags{};
            KMemoryManager::Pool        memory_pool{};
            s64                         schedule_count{};
            KCapabilities               capabilities{};
            ams::svc::ProgramId         program_id{};
            u64                         process_id{};
            s64                         creation_time{};
            KProcessAddress             code_address{};
            size_t                      code_size{};
            size_t                      main_thread_stack_size{};
            size_t                      max_process_memory{};
            u32                         version{};
            KHandleTable                handle_table{};
            KProcessAddress             plr_address{};
            void                       *plr_heap_address{};
            KThread                    *exception_thread{};
            ThreadList                  thread_list{};
            SharedMemoryInfoList        shared_memory_list{};
            BetaList                    beta_list{};
            bool                        is_suspended{};
            bool                        is_jit_debug{};
            ams::svc::DebugEvent        jit_debug_event_type{};
            ams::svc::DebugException    jit_debug_exception_type{};
            uintptr_t                   jit_debug_params[4]{};
            u64                         jit_debug_thread_id{};
            KWaitObject                 wait_object{};
            KThread                    *running_threads[cpu::NumCores]{};
            u64                         running_thread_idle_counts[cpu::NumCores]{};
            KThread                    *pinned_threads[cpu::NumCores]{};
            std::atomic<s32>            num_created_threads{};
            std::atomic<s64>            cpu_time{};
            std::atomic<s64>            num_process_switches{};
            std::atomic<s64>            num_thread_switches{};
            std::atomic<s64>            num_fpu_switches{};
            std::atomic<s64>            num_supervisor_calls{};
            std::atomic<s64>            num_ipc_messages{};
            std::atomic<s64>            num_ipc_replies{};
            std::atomic<s64>            num_ipc_receives{};
            KDynamicPageManager         dynamic_page_manager{};
            KMemoryBlockSlabManager     memory_block_slab_manager{};
            KBlockInfoManager           block_info_manager{};
            KPageTableManager           page_table_manager{};
        private:
            Result Initialize(const ams::svc::CreateProcessParameter &params);

            void StartTermination();
            void FinishTermination();

            void PinThread(s32 core_id, KThread *thread) {
                MESOSPHERE_ASSERT(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));
                MESOSPHERE_ASSERT(thread != nullptr);
                MESOSPHERE_ASSERT(this->pinned_threads[core_id] == nullptr);
                this->pinned_threads[core_id] = thread;
            }

            void UnpinThread(s32 core_id, KThread *thread) {
                MESOSPHERE_ASSERT(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));
                MESOSPHERE_ASSERT(thread != nullptr);
                MESOSPHERE_ASSERT(this->pinned_threads[core_id] == thread);
                this->pinned_threads[core_id] = nullptr;
            }
        public:
            KProcess() { /* ... */ }
            virtual ~KProcess() { /* ... */ }

            Result Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool);
            Result Initialize(const ams::svc::CreateProcessParameter &params, svc::KUserPointer<const u32 *> caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool);
            void Exit();

            constexpr const char *GetName() const { return this->name; }

            constexpr ams::svc::ProgramId GetProgramId() const { return this->program_id; }

            constexpr u64 GetProcessId() const { return this->process_id; }

            constexpr State GetState() const { return this->state; }

            constexpr u64 GetCoreMask() const { return this->capabilities.GetCoreMask(); }
            constexpr u64 GetPriorityMask() const { return this->capabilities.GetPriorityMask(); }

            constexpr s32 GetIdealCoreId() const { return this->ideal_core_id; }
            constexpr void SetIdealCoreId(s32 core_id) { this->ideal_core_id = core_id; }

            constexpr bool CheckThreadPriority(s32 prio) const { return ((1ul << prio) & this->GetPriorityMask()) != 0; }

            constexpr u32 GetCreateProcessFlags() const { return this->flags; }

            constexpr bool Is64Bit() const { return this->flags & ams::svc::CreateProcessFlag_Is64Bit; }

            constexpr KProcessAddress GetEntryPoint() const { return this->code_address; }

            constexpr size_t GetMainStackSize() const { return this->main_thread_stack_size; }

            constexpr KMemoryManager::Pool GetMemoryPool() const { return this->memory_pool; }

            constexpr u64 GetRandomEntropy(size_t i) const { return this->entropy[i]; }

            constexpr bool IsApplication() const { return this->is_application; }

            constexpr bool IsSuspended() const { return this->is_suspended; }
            constexpr void SetSuspended(bool suspended) { this->is_suspended = suspended; }

            Result Terminate();

            constexpr bool IsTerminated() const {
                return this->state == State_Terminated;
            }

            constexpr bool IsAttachedToDebugger() const {
                return this->attached_object != nullptr;
            }

            constexpr bool IsPermittedInterrupt(int32_t interrupt_id) const {
                return this->capabilities.IsPermittedInterrupt(interrupt_id);
            }

            constexpr bool IsPermittedDebug() const {
                return this->capabilities.IsPermittedDebug();
            }

            constexpr bool CanForceDebug() const {
                return this->capabilities.CanForceDebug();
            }

            u32 GetAllocateOption() const { return this->page_table.GetAllocateOption(); }

            ThreadList &GetThreadList() { return this->thread_list; }
            const ThreadList &GetThreadList() const { return this->thread_list; }

            constexpr void *GetDebugObject() const { return this->attached_object; }
            KProcess::State SetDebugObject(void *debug_object);
            void ClearDebugObject(KProcess::State state);

            bool EnterJitDebug(ams::svc::DebugEvent event, ams::svc::DebugException exception, uintptr_t param1 = 0, uintptr_t param2 = 0, uintptr_t param3 = 0, uintptr_t param4 = 0);

            KEventInfo *GetJitDebugInfo();
            void ClearJitDebugInfo();

            bool EnterUserException();
            bool LeaveUserException();
            bool ReleaseUserException(KThread *thread);

            KThread *GetPinnedThread(s32 core_id) const {
                MESOSPHERE_ASSERT(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));
                return this->pinned_threads[core_id];
            }

            void CopySvcPermissionsTo(KThread::StackParameters &sp) {
                this->capabilities.CopySvcPermissionsTo(sp);
            }

            void CopyPinnedSvcPermissionsTo(KThread::StackParameters &sp) {
                this->capabilities.CopyPinnedSvcPermissionsTo(sp);
            }

            void CopyUnpinnedSvcPermissionsTo(KThread::StackParameters &sp) {
                this->capabilities.CopyUnpinnedSvcPermissionsTo(sp);
            }

            void CopyEnterExceptionSvcPermissionsTo(KThread::StackParameters &sp) {
                this->capabilities.CopyEnterExceptionSvcPermissionsTo(sp);
            }

            void CopyLeaveExceptionSvcPermissionsTo(KThread::StackParameters &sp) {
                this->capabilities.CopyLeaveExceptionSvcPermissionsTo(sp);
            }

            constexpr KResourceLimit *GetResourceLimit() const { return this->resource_limit; }

            bool ReserveResource(ams::svc::LimitableResource which, s64 value);
            bool ReserveResource(ams::svc::LimitableResource which, s64 value, s64 timeout);
            void ReleaseResource(ams::svc::LimitableResource which, s64 value);
            void ReleaseResource(ams::svc::LimitableResource which, s64 value, s64 hint);

            constexpr KLightLock &GetStateLock() { return this->state_lock; }
            constexpr KLightLock &GetListLock() { return this->list_lock; }

            constexpr KProcessPageTable &GetPageTable() { return this->page_table; }
            constexpr const KProcessPageTable &GetPageTable() const { return this->page_table; }

            constexpr KHandleTable &GetHandleTable() { return this->handle_table; }
            constexpr const KHandleTable &GetHandleTable() const { return this->handle_table; }

            KWaitObject *GetWaitObjectPointer() { return std::addressof(this->wait_object); }

            size_t GetUsedUserPhysicalMemorySize() const;
            size_t GetTotalUserPhysicalMemorySize() const;
            size_t GetUsedNonSystemUserPhysicalMemorySize() const;
            size_t GetTotalNonSystemUserPhysicalMemorySize() const;

            Result AddSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size);
            void RemoveSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size);

            Result CreateThreadLocalRegion(KProcessAddress *out);
            Result DeleteThreadLocalRegion(KProcessAddress addr);
            void *GetThreadLocalRegionPointer(KProcessAddress addr);

            constexpr KProcessAddress GetProcessLocalRegionAddress() const { return this->plr_address; }

            void AddCpuTime(s64 diff) { this->cpu_time += diff; }
            s64 GetCpuTime() { return this->cpu_time; }

            constexpr s64 GetScheduledCount() const { return this->schedule_count; }
            void IncrementScheduledCount() { ++this->schedule_count; }

            void IncrementThreadCount();
            void DecrementThreadCount();

            size_t GetTotalSystemResourceSize() const { return this->system_resource_num_pages * PageSize; }
            size_t GetUsedSystemResourceSize() const {
                if (this->system_resource_num_pages == 0) {
                    return 0;
                }
                return this->dynamic_page_manager.GetUsed() * PageSize;
            }

            void SetRunningThread(s32 core, KThread *thread, u64 idle_count) {
                this->running_threads[core]            = thread;
                this->running_thread_idle_counts[core] = idle_count;
            }

            void ClearRunningThread(KThread *thread) {
                for (size_t i = 0; i < util::size(this->running_threads); ++i) {
                    if (this->running_threads[i] == thread) {
                        this->running_threads[i] = nullptr;
                    }
                }
            }

            const KDynamicPageManager &GetDynamicPageManager() const { return this->dynamic_page_manager; }
            const KMemoryBlockSlabManager &GetMemoryBlockSlabManager() const { return this->memory_block_slab_manager; }
            const KBlockInfoManager &GetBlockInfoManager() const { return this->block_info_manager; }
            const KPageTableManager &GetPageTableManager() const { return this->page_table_manager; }

            constexpr KThread *GetRunningThread(s32 core) const { return this->running_threads[core]; }
            constexpr u64 GetRunningThreadIdleCount(s32 core) const { return this->running_thread_idle_counts[core]; }

            void RegisterThread(KThread *thread);
            void UnregisterThread(KThread *thread);

            Result Run(s32 priority, size_t stack_size);

            Result Reset();

            void SetDebugBreak() {
                if (this->state == State_RunningAttached) {
                    this->ChangeState(State_DebugBreak);
                }
            }

            void SetAttached() {
                if (this->state == State_DebugBreak) {
                    this->ChangeState(State_RunningAttached);
                }
            }

            Result SetActivity(ams::svc::ProcessActivity activity);

            void PinCurrentThread();
            void UnpinCurrentThread();

            Result SignalToAddress(KProcessAddress address) {
                return this->cond_var.SignalToAddress(address);
            }

            Result WaitForAddress(ams::svc::Handle handle, KProcessAddress address, u32 tag) {
                return this->cond_var.WaitForAddress(handle, address, tag);
            }

            void SignalConditionVariable(uintptr_t cv_key, int32_t count) {
                return this->cond_var.Signal(cv_key, count);
            }

            Result WaitConditionVariable(KProcessAddress address, uintptr_t cv_key, u32 tag, s64 ns) {
                return this->cond_var.Wait(address, cv_key, tag, ns);
            }

            Result SignalAddressArbiter(uintptr_t address, ams::svc::SignalType signal_type, s32 value, s32 count) {
                return this->address_arbiter.SignalToAddress(address, signal_type, value, count);
            }

            Result WaitAddressArbiter(uintptr_t address, ams::svc::ArbitrationType arb_type, s32 value, s64 timeout) {
                return this->address_arbiter.WaitForAddress(address, arb_type, value, timeout);
            }

            Result GetThreadList(s32 *out_num_threads, ams::kern::svc::KUserPointer<u64 *> out_thread_ids, s32 max_out_count);

            static KProcess *GetProcessFromId(u64 process_id);
            static Result GetProcessList(s32 *out_num_processes, ams::kern::svc::KUserPointer<u64 *> out_process_ids, s32 max_out_count);

            static void Switch(KProcess *cur_process, KProcess *next_process) {
                MESOSPHERE_UNUSED(cur_process);

                /* Update the current page table. */
                if (next_process) {
                    next_process->GetPageTable().Activate(next_process->GetProcessId());
                } else {
                    Kernel::GetKernelPageTable().Activate();
                }
            }
        public:
            /* Overridden parent functions. */
            virtual bool IsInitialized() const override { return this->is_initialized; }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            virtual void Finalize() override;

            virtual u64 GetId() const override final { return this->GetProcessId(); }

            virtual bool IsSignaled() const override {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
                return this->is_signaled;
            }

            virtual void DoWorkerTask() override;
        private:
            void ChangeState(State new_state) {
                if (this->state != new_state) {
                    this->state = new_state;
                    this->is_signaled = true;
                    this->NotifyAvailable();
                }
            }
    };

}
