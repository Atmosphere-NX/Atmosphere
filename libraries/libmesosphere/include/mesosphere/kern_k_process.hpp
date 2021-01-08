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

            using ThreadList = util::IntrusiveListMemberTraits<&KThread::m_process_list_node>::ListType;

            static constexpr size_t AslrAlignment = KernelAslrAlignment;
        private:
            using SharedMemoryInfoList = util::IntrusiveListBaseTraits<KSharedMemoryInfo>::ListType;
            using BetaList = util::IntrusiveListMemberTraits<&KBeta::m_process_list_node>::ListType;
            using TLPTree = util::IntrusiveRedBlackTreeBaseTraits<KThreadLocalPage>::TreeType<KThreadLocalPage>;
            using TLPIterator = TLPTree::iterator;
        private:
            KProcessPageTable           m_page_table{};
            std::atomic<size_t>         m_used_kernel_memory_size{};
            TLPTree                     m_fully_used_tlp_tree{};
            TLPTree                     m_partially_used_tlp_tree{};
            s32                         m_ideal_core_id{};
            void                       *m_attached_object{};
            KResourceLimit             *m_resource_limit{};
            KVirtualAddress             m_system_resource_address{};
            size_t                      m_system_resource_num_pages{};
            size_t                      m_memory_release_hint{};
            State                       m_state{};
            KLightLock                  m_state_lock{};
            KLightLock                  m_list_lock{};
            KConditionVariable          m_cond_var{};
            KAddressArbiter             m_address_arbiter{};
            u64                         m_entropy[4]{};
            bool                        m_is_signaled{};
            bool                        m_is_initialized{};
            bool                        m_is_application{};
            char                        m_name[13]{};
            std::atomic<u16>            m_num_threads{};
            u16                         m_peak_num_threads{};
            u32                         m_flags{};
            KMemoryManager::Pool        m_memory_pool{};
            s64                         m_schedule_count{};
            KCapabilities               m_capabilities{};
            ams::svc::ProgramId         m_program_id{};
            u64                         m_process_id{};
            s64                         m_creation_time{};
            KProcessAddress             m_code_address{};
            size_t                      m_code_size{};
            size_t                      m_main_thread_stack_size{};
            size_t                      m_max_process_memory{};
            u32                         m_version{};
            KHandleTable                m_handle_table{};
            KProcessAddress             m_plr_address{};
            void                       *m_plr_heap_address{};
            KThread                    *m_exception_thread{};
            ThreadList                  m_thread_list{};
            SharedMemoryInfoList        m_shared_memory_list{};
            BetaList                    m_beta_list{};
            bool                        m_is_suspended{};
            bool                        m_is_jit_debug{};
            ams::svc::DebugEvent        m_jit_debug_event_type{};
            ams::svc::DebugException    m_jit_debug_exception_type{};
            uintptr_t                   m_jit_debug_params[4]{};
            u64                         m_jit_debug_thread_id{};
            KWaitObject                 m_wait_object{};
            KThread                    *m_running_threads[cpu::NumCores]{};
            u64                         m_running_thread_idle_counts[cpu::NumCores]{};
            KThread                    *m_pinned_threads[cpu::NumCores]{};
            std::atomic<s32>            m_num_created_threads{};
            std::atomic<s64>            m_cpu_time{};
            std::atomic<s64>            m_num_process_switches{};
            std::atomic<s64>            m_num_thread_switches{};
            std::atomic<s64>            m_num_fpu_switches{};
            std::atomic<s64>            m_num_supervisor_calls{};
            std::atomic<s64>            m_num_ipc_messages{};
            std::atomic<s64>            m_num_ipc_replies{};
            std::atomic<s64>            m_num_ipc_receives{};
            KDynamicPageManager         m_dynamic_page_manager{};
            KMemoryBlockSlabManager     m_memory_block_slab_manager{};
            KBlockInfoManager           m_block_info_manager{};
            KPageTableManager           m_page_table_manager{};
        private:
            Result Initialize(const ams::svc::CreateProcessParameter &params);

            void StartTermination();
            void FinishTermination();

            void PinThread(s32 core_id, KThread *thread) {
                MESOSPHERE_ASSERT(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));
                MESOSPHERE_ASSERT(thread != nullptr);
                MESOSPHERE_ASSERT(m_pinned_threads[core_id] == nullptr);
                m_pinned_threads[core_id] = thread;
            }

            void UnpinThread(s32 core_id, KThread *thread) {
                MESOSPHERE_UNUSED(thread);
                MESOSPHERE_ASSERT(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));
                MESOSPHERE_ASSERT(thread != nullptr);
                MESOSPHERE_ASSERT(m_pinned_threads[core_id] == thread);
                m_pinned_threads[core_id] = nullptr;
            }
        public:
            KProcess() { /* ... */ }
            virtual ~KProcess() { /* ... */ }

            Result Initialize(const ams::svc::CreateProcessParameter &params, const KPageGroup &pg, const u32 *caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool);
            Result Initialize(const ams::svc::CreateProcessParameter &params, svc::KUserPointer<const u32 *> caps, s32 num_caps, KResourceLimit *res_limit, KMemoryManager::Pool pool);
            void Exit();

            constexpr const char *GetName() const { return m_name; }

            constexpr ams::svc::ProgramId GetProgramId() const { return m_program_id; }

            constexpr u64 GetProcessId() const { return m_process_id; }

            constexpr State GetState() const { return m_state; }

            constexpr u64 GetCoreMask() const { return m_capabilities.GetCoreMask(); }
            constexpr u64 GetPriorityMask() const { return m_capabilities.GetPriorityMask(); }

            constexpr s32 GetIdealCoreId() const { return m_ideal_core_id; }
            constexpr void SetIdealCoreId(s32 core_id) { m_ideal_core_id = core_id; }

            constexpr bool CheckThreadPriority(s32 prio) const { return ((1ul << prio) & this->GetPriorityMask()) != 0; }

            constexpr u32 GetCreateProcessFlags() const { return m_flags; }

            constexpr bool Is64Bit() const { return m_flags & ams::svc::CreateProcessFlag_Is64Bit; }

            constexpr KProcessAddress GetEntryPoint() const { return m_code_address; }

            constexpr size_t GetMainStackSize() const { return m_main_thread_stack_size; }

            constexpr KMemoryManager::Pool GetMemoryPool() const { return m_memory_pool; }

            constexpr u64 GetRandomEntropy(size_t i) const { return m_entropy[i]; }

            constexpr bool IsApplication() const { return m_is_application; }

            constexpr bool IsSuspended() const { return m_is_suspended; }
            constexpr void SetSuspended(bool suspended) { m_is_suspended = suspended; }

            Result Terminate();

            constexpr bool IsTerminated() const {
                return m_state == State_Terminated;
            }

            constexpr bool IsAttachedToDebugger() const {
                return m_attached_object != nullptr;
            }

            constexpr bool IsPermittedInterrupt(int32_t interrupt_id) const {
                return m_capabilities.IsPermittedInterrupt(interrupt_id);
            }

            constexpr bool IsPermittedDebug() const {
                return m_capabilities.IsPermittedDebug();
            }

            constexpr bool CanForceDebug() const {
                return m_capabilities.CanForceDebug();
            }

            u32 GetAllocateOption() const { return m_page_table.GetAllocateOption(); }

            ThreadList &GetThreadList() { return m_thread_list; }
            const ThreadList &GetThreadList() const { return m_thread_list; }

            constexpr void *GetDebugObject() const { return m_attached_object; }
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
                return m_pinned_threads[core_id];
            }

            void CopySvcPermissionsTo(KThread::StackParameters &sp) {
                m_capabilities.CopySvcPermissionsTo(sp);
            }

            void CopyPinnedSvcPermissionsTo(KThread::StackParameters &sp) {
                m_capabilities.CopyPinnedSvcPermissionsTo(sp);
            }

            void CopyUnpinnedSvcPermissionsTo(KThread::StackParameters &sp) {
                m_capabilities.CopyUnpinnedSvcPermissionsTo(sp);
            }

            void CopyEnterExceptionSvcPermissionsTo(KThread::StackParameters &sp) {
                m_capabilities.CopyEnterExceptionSvcPermissionsTo(sp);
            }

            void CopyLeaveExceptionSvcPermissionsTo(KThread::StackParameters &sp) {
                m_capabilities.CopyLeaveExceptionSvcPermissionsTo(sp);
            }

            constexpr KResourceLimit *GetResourceLimit() const { return m_resource_limit; }

            bool ReserveResource(ams::svc::LimitableResource which, s64 value);
            bool ReserveResource(ams::svc::LimitableResource which, s64 value, s64 timeout);
            void ReleaseResource(ams::svc::LimitableResource which, s64 value);
            void ReleaseResource(ams::svc::LimitableResource which, s64 value, s64 hint);

            constexpr KLightLock &GetStateLock() { return m_state_lock; }
            constexpr KLightLock &GetListLock() { return m_list_lock; }

            constexpr KProcessPageTable &GetPageTable() { return m_page_table; }
            constexpr const KProcessPageTable &GetPageTable() const { return m_page_table; }

            constexpr KHandleTable &GetHandleTable() { return m_handle_table; }
            constexpr const KHandleTable &GetHandleTable() const { return m_handle_table; }

            KWaitObject *GetWaitObjectPointer() { return std::addressof(m_wait_object); }

            size_t GetUsedUserPhysicalMemorySize() const;
            size_t GetTotalUserPhysicalMemorySize() const;
            size_t GetUsedNonSystemUserPhysicalMemorySize() const;
            size_t GetTotalNonSystemUserPhysicalMemorySize() const;

            Result AddSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size);
            void RemoveSharedMemory(KSharedMemory *shmem, KProcessAddress address, size_t size);

            Result CreateThreadLocalRegion(KProcessAddress *out);
            Result DeleteThreadLocalRegion(KProcessAddress addr);
            void *GetThreadLocalRegionPointer(KProcessAddress addr);

            constexpr KProcessAddress GetProcessLocalRegionAddress() const { return m_plr_address; }

            void AddCpuTime(s64 diff) { m_cpu_time += diff; }
            s64 GetCpuTime() { return m_cpu_time; }

            constexpr s64 GetScheduledCount() const { return m_schedule_count; }
            void IncrementScheduledCount() { ++m_schedule_count; }

            void IncrementThreadCount();
            void DecrementThreadCount();

            size_t GetTotalSystemResourceSize() const { return m_system_resource_num_pages * PageSize; }
            size_t GetUsedSystemResourceSize() const {
                if (m_system_resource_num_pages == 0) {
                    return 0;
                }
                return m_dynamic_page_manager.GetUsed() * PageSize;
            }

            void SetRunningThread(s32 core, KThread *thread, u64 idle_count) {
                m_running_threads[core]            = thread;
                m_running_thread_idle_counts[core] = idle_count;
            }

            void ClearRunningThread(KThread *thread) {
                for (size_t i = 0; i < util::size(m_running_threads); ++i) {
                    if (m_running_threads[i] == thread) {
                        m_running_threads[i] = nullptr;
                    }
                }
            }

            const KDynamicPageManager &GetDynamicPageManager() const { return m_dynamic_page_manager; }
            const KMemoryBlockSlabManager &GetMemoryBlockSlabManager() const { return m_memory_block_slab_manager; }
            const KBlockInfoManager &GetBlockInfoManager() const { return m_block_info_manager; }
            const KPageTableManager &GetPageTableManager() const { return m_page_table_manager; }

            constexpr KThread *GetRunningThread(s32 core) const { return m_running_threads[core]; }
            constexpr u64 GetRunningThreadIdleCount(s32 core) const { return m_running_thread_idle_counts[core]; }

            void RegisterThread(KThread *thread);
            void UnregisterThread(KThread *thread);

            Result Run(s32 priority, size_t stack_size);

            Result Reset();

            void SetDebugBreak() {
                if (m_state == State_RunningAttached) {
                    this->ChangeState(State_DebugBreak);
                }
            }

            void SetAttached() {
                if (m_state == State_DebugBreak) {
                    this->ChangeState(State_RunningAttached);
                }
            }

            Result SetActivity(ams::svc::ProcessActivity activity);

            void PinCurrentThread();
            void UnpinCurrentThread();

            Result SignalToAddress(KProcessAddress address) {
                return m_cond_var.SignalToAddress(address);
            }

            Result WaitForAddress(ams::svc::Handle handle, KProcessAddress address, u32 tag) {
                return m_cond_var.WaitForAddress(handle, address, tag);
            }

            void SignalConditionVariable(uintptr_t cv_key, int32_t count) {
                return m_cond_var.Signal(cv_key, count);
            }

            Result WaitConditionVariable(KProcessAddress address, uintptr_t cv_key, u32 tag, s64 ns) {
                return m_cond_var.Wait(address, cv_key, tag, ns);
            }

            Result SignalAddressArbiter(uintptr_t address, ams::svc::SignalType signal_type, s32 value, s32 count) {
                return m_address_arbiter.SignalToAddress(address, signal_type, value, count);
            }

            Result WaitAddressArbiter(uintptr_t address, ams::svc::ArbitrationType arb_type, s32 value, s64 timeout) {
                return m_address_arbiter.WaitForAddress(address, arb_type, value, timeout);
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
            virtual bool IsInitialized() const override { return m_is_initialized; }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            virtual void Finalize() override;

            virtual u64 GetId() const override final { return this->GetProcessId(); }

            virtual bool IsSignaled() const override {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
                return m_is_signaled;
            }

            virtual void DoWorkerTask() override;
        private:
            void ChangeState(State new_state) {
                if (m_state != new_state) {
                    m_state = new_state;
                    m_is_signaled = true;
                    this->NotifyAvailable();
                }
            }
    };

}
