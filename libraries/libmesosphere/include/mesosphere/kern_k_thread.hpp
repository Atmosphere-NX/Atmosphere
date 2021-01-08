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
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_affinity_mask.hpp>
#include <mesosphere/kern_k_thread_context.hpp>
#include <mesosphere/kern_k_current_context.hpp>
#include <mesosphere/kern_k_timer_task.hpp>
#include <mesosphere/kern_k_worker_task.hpp>

namespace ams::kern {

    class KThreadQueue;
    class KProcess;
    class KConditionVariable;
    class KAddressArbiter;

    using KThreadFunction = void (*)(uintptr_t);

    class KThread final : public KAutoObjectWithSlabHeapAndContainer<KThread, KSynchronizationObject>, public util::IntrusiveListBaseNode<KThread>, public KTimerTask, public KWorkerTask {
        MESOSPHERE_AUTOOBJECT_TRAITS(KThread, KSynchronizationObject);
        private:
            friend class KProcess;
            friend class KConditionVariable;
            friend class KAddressArbiter;
        public:
            static constexpr s32 MainThreadPriority = 1;
            static constexpr s32 IdleThreadPriority = 64;

            enum ThreadType : u32 {
                ThreadType_Main         = 0,
                ThreadType_Kernel       = 1,
                ThreadType_HighPriority = 2,
                ThreadType_User         = 3,
            };

            enum SuspendType : u32 {
                SuspendType_Process   = 0,
                SuspendType_Thread    = 1,
                SuspendType_Debug     = 2,
                SuspendType_Backtrace = 3,
                SuspendType_Init      = 4,

                SuspendType_Count,
            };

            enum ThreadState : u16 {
                ThreadState_Initialized = 0,
                ThreadState_Waiting     = 1,
                ThreadState_Runnable    = 2,
                ThreadState_Terminated  = 3,

                ThreadState_SuspendShift = 4,
                ThreadState_Mask         = (1 << ThreadState_SuspendShift) - 1,

                ThreadState_ProcessSuspended   = (1 << (SuspendType_Process   + ThreadState_SuspendShift)),
                ThreadState_ThreadSuspended    = (1 << (SuspendType_Thread    + ThreadState_SuspendShift)),
                ThreadState_DebugSuspended     = (1 << (SuspendType_Debug     + ThreadState_SuspendShift)),
                ThreadState_BacktraceSuspended = (1 << (SuspendType_Backtrace + ThreadState_SuspendShift)),
                ThreadState_InitSuspended      = (1 << (SuspendType_Init      + ThreadState_SuspendShift)),

                ThreadState_SuspendFlagMask    = ((1 << SuspendType_Count) - 1) << ThreadState_SuspendShift,
            };

            enum DpcFlag : u32 {
                DpcFlag_Terminating = (1 << 0),
                DpcFlag_Terminated  = (1 << 1),
            };

            struct StackParameters {
                alignas(0x10) u8 svc_permission[0x10];
                std::atomic<u8> dpc_flags;
                u8 current_svc_id;
                bool is_calling_svc;
                bool is_in_exception_handler;
                bool is_pinned;
                s32 disable_count;
                KThreadContext *context;
                KThread *cur_thread;
            };
            static_assert(alignof(StackParameters) == 0x10);

            struct QueueEntry {
                private:
                    KThread *m_prev;
                    KThread *m_next;
                public:
                    constexpr QueueEntry() : m_prev(nullptr), m_next(nullptr) { /* ... */ }

                    constexpr void Initialize() {
                        m_prev = nullptr;
                        m_next = nullptr;
                    }

                    constexpr KThread *GetPrev() const { return m_prev; }
                    constexpr KThread *GetNext() const { return m_next; }
                    constexpr void SetPrev(KThread *t) { m_prev = t; }
                    constexpr void SetNext(KThread *t) { m_next = t; }
            };

            using WaiterList = util::IntrusiveListBaseTraits<KThread>::ListType;
        private:
            static constexpr size_t PriorityInheritanceCountMax = 10;
            union SyncObjectBuffer {
                KSynchronizationObject *m_sync_objects[ams::svc::ArgumentHandleCountMax];
                ams::svc::Handle        m_handles[ams::svc::ArgumentHandleCountMax * (sizeof(KSynchronizationObject *) / sizeof(ams::svc::Handle))];

                constexpr SyncObjectBuffer() : m_sync_objects() { /* ... */ }
            };
            static_assert(sizeof(SyncObjectBuffer::m_sync_objects) == sizeof(SyncObjectBuffer::m_handles));

            struct ConditionVariableComparator {
                struct LightCompareType {
                    uintptr_t m_cv_key;
                    s32 m_priority;

                    constexpr ALWAYS_INLINE uintptr_t GetConditionVariableKey() const {
                        return m_cv_key;
                    }

                    constexpr ALWAYS_INLINE s32 GetPriority() const {
                        return m_priority;
                    }
                };

                template<typename T> requires (std::same_as<T, KThread> || std::same_as<T, LightCompareType>)
                static constexpr ALWAYS_INLINE int Compare(const T &lhs, const KThread &rhs) {
                    const uintptr_t l_key = lhs.GetConditionVariableKey();
                    const uintptr_t r_key = rhs.GetConditionVariableKey();

                    if (l_key < r_key) {
                        /* Sort first by key */
                        return -1;
                    } else if (l_key == r_key && lhs.GetPriority() < rhs.GetPriority()) {
                        /* And then by priority. */
                        return -1;
                    } else {
                        return 1;
                    }
                }
            };
            static_assert(ams::util::HasLightCompareType<ConditionVariableComparator>);
            static_assert(std::same_as<ams::util::LightCompareType<ConditionVariableComparator, void>, ConditionVariableComparator::LightCompareType>);
        private:
            static inline std::atomic<u64> s_next_thread_id = 0;
        private:
            alignas(16) KThreadContext      m_thread_context{};
            util::IntrusiveListNode         m_process_list_node{};
            util::IntrusiveRedBlackTreeNode m_condvar_arbiter_tree_node{};
            s32                             m_priority{};

            using ConditionVariableThreadTreeTraits = util::IntrusiveRedBlackTreeMemberTraitsDeferredAssert<&KThread::m_condvar_arbiter_tree_node>;
            using ConditionVariableThreadTree       = ConditionVariableThreadTreeTraits::TreeType<ConditionVariableComparator>;

            ConditionVariableThreadTree    *m_condvar_tree{};
            uintptr_t                       m_condvar_key{};
            u64                             m_virtual_affinity_mask{};
            KAffinityMask                   m_physical_affinity_mask{};
            u64                             m_thread_id{};
            std::atomic<s64>                m_cpu_time{};
            KSynchronizationObject         *m_synced_object{};
            KProcessAddress                 m_address_key{};
            KProcess                       *m_parent{};
            void                           *m_kernel_stack_top{};
            u32                            *m_light_ipc_data{};
            KProcessAddress                 m_tls_address{};
            void                           *m_tls_heap_address{};
            KLightLock                      m_activity_pause_lock{};
            SyncObjectBuffer                m_sync_object_buffer{};
            s64                             m_schedule_count{};
            s64                             m_last_scheduled_tick{};
            QueueEntry                      m_per_core_priority_queue_entry[cpu::NumCores]{};
            KLightLock                     *m_waiting_lock{};

            KThreadQueue                   *m_sleeping_queue{};

            WaiterList                      m_waiter_list{};
            WaiterList                      m_pinned_waiter_list{};
            KThread                        *m_lock_owner{};
            uintptr_t                       m_debug_params[3]{};
            u32                             m_address_key_value{};
            u32                             m_suspend_request_flags{};
            u32                             m_suspend_allowed_flags{};
            Result                          m_wait_result;
            Result                          m_debug_exception_result;
            s32                             m_base_priority{};
            s32                             m_physical_ideal_core_id{};
            s32                             m_virtual_ideal_core_id{};
            s32                             m_num_kernel_waiters{};
            s32                             m_current_core_id{};
            s32                             m_core_id{};
            KAffinityMask                   m_original_physical_affinity_mask{};
            s32                             m_original_physical_ideal_core_id{};
            s32                             m_num_core_migration_disables{};
            ThreadState                     m_thread_state{};
            std::atomic<u8>                 m_termination_requested{};
            bool                            m_wait_cancelled{};
            bool                            m_cancellable{};
            bool                            m_signaled{};
            bool                            m_initialized{};
            bool                            m_debug_attached{};
            s8                              m_priority_inheritance_count{};
            bool                            m_resource_limit_release_hint{};
        public:
            constexpr KThread() : m_wait_result(svc::ResultNoSynchronizationObject()), m_debug_exception_result(ResultSuccess()) { /* ... */ }

            virtual ~KThread() { /* ... */ }

            Result Initialize(KThreadFunction func, uintptr_t arg, void *kern_stack_top, KProcessAddress user_stack_top, s32 prio, s32 virt_core, KProcess *owner, ThreadType type);

        private:
            static Result InitializeThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 virt_core, KProcess *owner, ThreadType type);
        public:
            static Result InitializeKernelThread(KThread *thread, KThreadFunction func, uintptr_t arg, s32 prio, s32 virt_core) {
                return InitializeThread(thread, func, arg, Null<KProcessAddress>, prio, virt_core, nullptr, ThreadType_Kernel);
            }

            static Result InitializeHighPriorityThread(KThread *thread, KThreadFunction func, uintptr_t arg) {
                return InitializeThread(thread, func, arg, Null<KProcessAddress>, 0, GetCurrentCoreId(), nullptr, ThreadType_HighPriority);
            }

            static Result InitializeUserThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 virt_core, KProcess *owner) {
                return InitializeThread(thread, func, arg, user_stack_top, prio, virt_core, owner, ThreadType_User);
            }

            static void ResumeThreadsSuspendedForInit();
        private:
            StackParameters &GetStackParameters() {
                return *(reinterpret_cast<StackParameters *>(m_kernel_stack_top) - 1);
            }

            const StackParameters &GetStackParameters() const {
                return *(reinterpret_cast<const StackParameters *>(m_kernel_stack_top) - 1);
            }
        public:
            StackParameters &GetStackParametersForExceptionSvcPermission() {
                return *(reinterpret_cast<StackParameters *>(m_kernel_stack_top) - 1);
            }
        public:
            ALWAYS_INLINE s32 GetDisableDispatchCount() const {
                MESOSPHERE_ASSERT_THIS();
                return this->GetStackParameters().disable_count;
            }

            ALWAYS_INLINE void DisableDispatch() {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() >= 0);
                this->GetStackParameters().disable_count++;
            }

            ALWAYS_INLINE void EnableDispatch() {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(GetCurrentThread().GetDisableDispatchCount() >  0);
                this->GetStackParameters().disable_count--;
            }

            void Pin();
            void Unpin();

            ALWAYS_INLINE void SaveDebugParams(uintptr_t param1, uintptr_t param2, uintptr_t param3) {
                m_debug_params[0] = param1;
                m_debug_params[1] = param2;
                m_debug_params[2] = param3;
            }

            ALWAYS_INLINE void RestoreDebugParams(uintptr_t *param1, uintptr_t *param2, uintptr_t *param3) {
                *param1 = m_debug_params[0];
                *param2 = m_debug_params[1];
                *param3 = m_debug_params[2];
            }

            NOINLINE void DisableCoreMigration();
            NOINLINE void EnableCoreMigration();

            ALWAYS_INLINE void SetInExceptionHandler() {
                MESOSPHERE_ASSERT_THIS();
                this->GetStackParameters().is_in_exception_handler = true;
            }

            ALWAYS_INLINE void ClearInExceptionHandler() {
                MESOSPHERE_ASSERT_THIS();
                this->GetStackParameters().is_in_exception_handler = false;
            }

            ALWAYS_INLINE bool IsInExceptionHandler() const {
                MESOSPHERE_ASSERT_THIS();
                return this->GetStackParameters().is_in_exception_handler;
            }

            ALWAYS_INLINE bool IsCallingSvc() const {
                MESOSPHERE_ASSERT_THIS();
                return this->GetStackParameters().is_calling_svc;
            }

            ALWAYS_INLINE u8 GetSvcId() const {
                MESOSPHERE_ASSERT_THIS();
                return this->GetStackParameters().current_svc_id;
            }

            ALWAYS_INLINE void RegisterDpc(DpcFlag flag) {
                this->GetStackParameters().dpc_flags |= flag;
            }

            ALWAYS_INLINE void ClearDpc(DpcFlag flag) {
                this->GetStackParameters().dpc_flags &= ~flag;
            }

            ALWAYS_INLINE u8 GetDpc() const {
                return this->GetStackParameters().dpc_flags;
            }

            ALWAYS_INLINE bool HasDpc() const {
                MESOSPHERE_ASSERT_THIS();
                return this->GetDpc() != 0;
            }
        private:
            void Suspend();
            ALWAYS_INLINE void AddWaiterImpl(KThread *thread);
            ALWAYS_INLINE void RemoveWaiterImpl(KThread *thread);
            ALWAYS_INLINE static void RestorePriority(KThread *thread);

            void StartTermination();
            void FinishTermination();
        public:
            constexpr u64 GetThreadId() const { return m_thread_id; }

            constexpr KThreadContext &GetContext() { return m_thread_context; }
            constexpr const KThreadContext &GetContext() const { return m_thread_context; }

            constexpr u64 GetVirtualAffinityMask() const { return m_virtual_affinity_mask; }
            constexpr const KAffinityMask &GetAffinityMask() const { return m_physical_affinity_mask; }

            Result GetCoreMask(int32_t *out_ideal_core, u64 *out_affinity_mask);
            Result SetCoreMask(int32_t ideal_core, u64 affinity_mask);

            Result GetPhysicalCoreMask(int32_t *out_ideal_core, u64 *out_affinity_mask);

            constexpr ThreadState GetState() const { return static_cast<ThreadState>(m_thread_state & ThreadState_Mask); }
            constexpr ThreadState GetRawState() const { return m_thread_state; }
            NOINLINE void SetState(ThreadState state);

            NOINLINE KThreadContext *GetContextForSchedulerLoop();

            constexpr uintptr_t GetConditionVariableKey() const { return m_condvar_key; }
            constexpr uintptr_t GetAddressArbiterKey() const { return m_condvar_key; }

            constexpr void SetConditionVariable(ConditionVariableThreadTree *tree, KProcessAddress address, uintptr_t cv_key, u32 value) {
                m_condvar_tree      = tree;
                m_condvar_key       = cv_key;
                m_address_key       = address;
                m_address_key_value = value;
            }

            constexpr void ClearConditionVariable() {
                m_condvar_tree = nullptr;
            }

            constexpr bool IsWaitingForConditionVariable() const {
                return m_condvar_tree != nullptr;
            }

            constexpr void SetAddressArbiter(ConditionVariableThreadTree *tree, uintptr_t address) {
                m_condvar_tree = tree;
                m_condvar_key  = address;
            }

            constexpr void ClearAddressArbiter() {
                m_condvar_tree = nullptr;
            }

            constexpr bool IsWaitingForAddressArbiter() const {
                return m_condvar_tree != nullptr;
            }

            constexpr s32 GetIdealVirtualCore() const { return m_virtual_ideal_core_id; }
            constexpr s32 GetIdealPhysicalCore() const { return m_physical_ideal_core_id; }

            constexpr s32 GetActiveCore() const { return m_core_id; }
            constexpr void SetActiveCore(s32 core) { m_core_id = core; }

            constexpr ALWAYS_INLINE s32 GetCurrentCore() const { return m_current_core_id; }
            constexpr void SetCurrentCore(s32 core) { m_current_core_id = core; }

            constexpr s32 GetPriority() const { return m_priority; }
            constexpr void SetPriority(s32 prio) { m_priority = prio; }

            constexpr s32 GetBasePriority() const { return m_base_priority; }

            constexpr QueueEntry &GetPriorityQueueEntry(s32 core) { return m_per_core_priority_queue_entry[core]; }
            constexpr const QueueEntry &GetPriorityQueueEntry(s32 core) const { return m_per_core_priority_queue_entry[core]; }

            constexpr void SetSleepingQueue(KThreadQueue *q) { m_sleeping_queue = q; }

            constexpr ConditionVariableThreadTree *GetConditionVariableTree() const { return m_condvar_tree; }

            constexpr s32 GetNumKernelWaiters() const { return m_num_kernel_waiters; }

            void AddWaiter(KThread *thread);
            void RemoveWaiter(KThread *thread);
            KThread *RemoveWaiterByKey(s32 *out_num_waiters, KProcessAddress key);

            constexpr KProcessAddress GetAddressKey() const { return m_address_key; }
            constexpr u32 GetAddressKeyValue() const { return m_address_key_value; }
            constexpr void SetAddressKey(KProcessAddress key) { m_address_key = key; }
            constexpr void SetAddressKey(KProcessAddress key, u32 val) { m_address_key = key; m_address_key_value = val; }

            constexpr void SetLockOwner(KThread *owner) { m_lock_owner = owner; }
            constexpr KThread *GetLockOwner() const { return m_lock_owner; }

            constexpr void SetSyncedObject(KSynchronizationObject *obj, Result wait_res) {
                MESOSPHERE_ASSERT_THIS();

                m_synced_object = obj;
                m_wait_result = wait_res;
            }

            constexpr Result GetWaitResult(KSynchronizationObject **out) const {
                MESOSPHERE_ASSERT_THIS();

                *out = m_synced_object;
                return m_wait_result;
            }

            constexpr void SetDebugExceptionResult(Result result) {
                MESOSPHERE_ASSERT_THIS();
                m_debug_exception_result = result;
            }

            constexpr Result GetDebugExceptionResult() const {
                MESOSPHERE_ASSERT_THIS();
                return m_debug_exception_result;
            }

            void WaitCancel();

            bool IsWaitCancelled() const { return m_wait_cancelled; }
            void ClearWaitCancelled() { m_wait_cancelled = false; }

            void ClearCancellable() { m_cancellable = false; }
            void SetCancellable() { m_cancellable = true; }

            constexpr u32 *GetLightSessionData() const { return m_light_ipc_data; }
            constexpr void SetLightSessionData(u32 *data) { m_light_ipc_data = data; }

            bool HasWaiters() const { return !m_waiter_list.empty(); }

            constexpr s64 GetLastScheduledTick() const { return m_last_scheduled_tick; }
            constexpr void SetLastScheduledTick(s64 tick) { m_last_scheduled_tick = tick; }

            constexpr s64 GetYieldScheduleCount() const { return m_schedule_count; }
            constexpr void SetYieldScheduleCount(s64 count) { m_schedule_count = count; }

            constexpr KProcess *GetOwnerProcess() const { return m_parent; }
            constexpr bool IsUserThread() const { return m_parent != nullptr; }

            constexpr KProcessAddress GetThreadLocalRegionAddress() const { return m_tls_address; }
            constexpr void           *GetThreadLocalRegionHeapAddress() const { return m_tls_heap_address; }

            constexpr KSynchronizationObject **GetSynchronizationObjectBuffer() { return std::addressof(m_sync_object_buffer.m_sync_objects[0]); }
            constexpr ams::svc::Handle *GetHandleBuffer() { return std::addressof(m_sync_object_buffer.m_handles[sizeof(m_sync_object_buffer.m_sync_objects) / sizeof(ams::svc::Handle) - ams::svc::ArgumentHandleCountMax]); }

            u16 GetUserDisableCount() const { return static_cast<ams::svc::ThreadLocalRegion *>(m_tls_heap_address)->disable_count; }
            void SetInterruptFlag()   const { static_cast<ams::svc::ThreadLocalRegion *>(m_tls_heap_address)->interrupt_flag = 1; }
            void ClearInterruptFlag() const { static_cast<ams::svc::ThreadLocalRegion *>(m_tls_heap_address)->interrupt_flag = 0; }

            constexpr void SetDebugAttached() { m_debug_attached = true; }
            constexpr bool IsAttachedToDebugger() const { return m_debug_attached; }

            void AddCpuTime(s32 core_id, s64 amount) {
                m_cpu_time += amount;
                /* TODO: Debug kernels track per-core tick counts. Should we? */
                MESOSPHERE_UNUSED(core_id);
            }

            s64 GetCpuTime() const { return m_cpu_time.load(); }

            s64 GetCpuTime(s32 core_id) const {
                MESOSPHERE_ABORT_UNLESS(0 <= core_id && core_id < static_cast<s32>(cpu::NumCores));

                /* TODO: Debug kernels track per-core tick counts. Should we? */
                return 0;
            }

            constexpr u32 GetSuspendFlags() const { return m_suspend_allowed_flags & m_suspend_request_flags; }
            constexpr bool IsSuspended() const { return this->GetSuspendFlags() != 0; }
            constexpr bool IsSuspendRequested(SuspendType type) const { return (m_suspend_request_flags & (1u << (ThreadState_SuspendShift + type))) != 0; }
            constexpr bool IsSuspendRequested() const { return m_suspend_request_flags != 0; }
            void RequestSuspend(SuspendType type);
            void Resume(SuspendType type);
            void TrySuspend();
            void Continue();

            Result SetActivity(ams::svc::ThreadActivity activity);
            Result GetThreadContext3(ams::svc::ThreadContext *out);

            void ContinueIfHasKernelWaiters() {
                if (this->GetNumKernelWaiters() > 0) {
                    this->Continue();
                }
            }

            void Wakeup();

            void SetBasePriority(s32 priority);
            Result SetPriorityToIdle();

            Result Run();
            void Exit();

            void Terminate();
            ThreadState RequestTerminate();

            Result Sleep(s64 timeout);

            ALWAYS_INLINE void *GetStackTop() const { return reinterpret_cast<StackParameters *>(m_kernel_stack_top) - 1; }
            ALWAYS_INLINE void *GetKernelStackTop() const { return m_kernel_stack_top; }

            ALWAYS_INLINE bool IsTerminationRequested() const {
                return m_termination_requested.load() || this->GetRawState() == ThreadState_Terminated;
            }

            size_t GetKernelStackUsage() const;
        public:
            /* Overridden parent functions. */
            virtual u64 GetId() const override final { return this->GetThreadId(); }

            virtual bool IsInitialized() const override { return m_initialized; }
            virtual uintptr_t GetPostDestroyArgument() const override { return reinterpret_cast<uintptr_t>(m_parent) | (m_resource_limit_release_hint ? 1 : 0); }

            static void PostDestroy(uintptr_t arg);

            virtual void Finalize() override;
            virtual bool IsSignaled() const override;
            virtual void OnTimer() override;
            virtual void DoWorkerTask() override;
        public:
            static constexpr bool IsConditionVariableThreadTreeValid() {
                return ConditionVariableThreadTreeTraits::IsValid();
            }

            static KThread *GetThreadFromId(u64 thread_id);
            static Result GetThreadList(s32 *out_num_threads, ams::kern::svc::KUserPointer<u64 *> out_thread_ids, s32 max_out_count);

            using ConditionVariableThreadTreeType = ConditionVariableThreadTree;
    };
    static_assert(alignof(KThread) == 0x10);
    static_assert(KThread::IsConditionVariableThreadTreeValid());

    class KScopedDisableDispatch {
        public:
            explicit ALWAYS_INLINE KScopedDisableDispatch() {
                GetCurrentThread().DisableDispatch();
            }

            ~KScopedDisableDispatch();
    };

    ALWAYS_INLINE KExceptionContext *GetExceptionContext(KThread *thread) {
        return reinterpret_cast<KExceptionContext *>(reinterpret_cast<uintptr_t>(thread->GetKernelStackTop()) - sizeof(KThread::StackParameters) - sizeof(KExceptionContext));
    }

    ALWAYS_INLINE const KExceptionContext *GetExceptionContext(const KThread *thread) {
        return reinterpret_cast<const KExceptionContext *>(reinterpret_cast<uintptr_t>(thread->GetKernelStackTop()) - sizeof(KThread::StackParameters) - sizeof(KExceptionContext));
    }

    ALWAYS_INLINE KProcess *GetCurrentProcessPointer() {
        return GetCurrentThread().GetOwnerProcess();
    }

    ALWAYS_INLINE KProcess &GetCurrentProcess() {
        return *GetCurrentProcessPointer();
    }

    ALWAYS_INLINE s32 GetCurrentCoreId() {
        return GetCurrentThread().GetCurrentCore();
    }

}
