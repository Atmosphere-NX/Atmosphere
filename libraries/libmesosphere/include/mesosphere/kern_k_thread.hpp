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

    class KThread final : public KAutoObjectWithSlabHeapAndContainer<KThread, KSynchronizationObject>, public KTimerTask, public KWorkerTask {
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
                SuspendType_Process = 0,
                SuspendType_Thread  = 1,
                SuspendType_Debug   = 2,
                SuspendType_Unk3    = 3,
                SuspendType_Init    = 4,

                SuspendType_Count,
            };

            enum ThreadState : u16 {
                ThreadState_Initialized = 0,
                ThreadState_Waiting     = 1,
                ThreadState_Runnable    = 2,
                ThreadState_Terminated  = 3,

                ThreadState_SuspendShift = 4,
                ThreadState_Mask         = (1 << ThreadState_SuspendShift) - 1,

                ThreadState_ProcessSuspended = (1 << (SuspendType_Process + ThreadState_SuspendShift)),
                ThreadState_ThreadSuspended  = (1 << (SuspendType_Thread  + ThreadState_SuspendShift)),
                ThreadState_DebugSuspended   = (1 << (SuspendType_Debug   + ThreadState_SuspendShift)),
                ThreadState_Unk3Suspended    = (1 << (SuspendType_Unk3    + ThreadState_SuspendShift)),
                ThreadState_InitSuspended    = (1 << (SuspendType_Init    + ThreadState_SuspendShift)),

                ThreadState_SuspendFlagMask  = ((1 << SuspendType_Count) - 1) << ThreadState_SuspendShift,
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
                bool is_preemption_state_pinned;
                s32 disable_count;
                KThreadContext *context;
            };
            static_assert(alignof(StackParameters) == 0x10);

            struct QueueEntry {
                private:
                    KThread *prev;
                    KThread *next;
                public:
                    constexpr QueueEntry() : prev(nullptr), next(nullptr) { /* ... */ }

                    constexpr void Initialize() {
                        this->prev = nullptr;
                        this->next = nullptr;
                    }

                    constexpr KThread *GetPrev() const { return this->prev; }
                    constexpr KThread *GetNext() const { return this->next; }
                    constexpr void SetPrev(KThread *t) { this->prev = t; }
                    constexpr void SetNext(KThread *t) { this->next = t; }
            };
        private:
            static constexpr size_t PriorityInheritanceCountMax = 10;
            union SyncObjectBuffer {
                KSynchronizationObject *sync_objects[ams::svc::MaxWaitSynchronizationHandleCount];
                ams::svc::Handle        handles[ams::svc::MaxWaitSynchronizationHandleCount * (sizeof(KSynchronizationObject *) / sizeof(ams::svc::Handle))];

                constexpr SyncObjectBuffer() : sync_objects() { /* ... */ }
            };
            static_assert(sizeof(SyncObjectBuffer::sync_objects) == sizeof(SyncObjectBuffer::handles));
        private:
            static inline std::atomic<u64> s_next_thread_id = 0;
        private:
            alignas(16) KThreadContext      thread_context{};
            KAffinityMask                   affinity_mask{};
            u64                             thread_id{};
            std::atomic<s64>                cpu_time{};
            KSynchronizationObject         *synced_object{};
            KLightLock                     *waiting_lock{};
            uintptr_t                       condvar_key{};
            uintptr_t                       entrypoint{};
            KProcessAddress                 arbiter_key{};
            KProcess                       *parent{};
            void                           *kernel_stack_top{};
            u32                            *light_ipc_data{};
            KProcessAddress                 tls_address{};
            void                           *tls_heap_address{};
            KLightLock                      activity_pause_lock{};
            SyncObjectBuffer                sync_object_buffer{};
            s64                             schedule_count{};
            s64                             last_scheduled_tick{};
            QueueEntry                      per_core_priority_queue_entry[cpu::NumCores]{};
            QueueEntry                      sleeping_queue_entry{};
            KThreadQueue                   *sleeping_queue{};
            util::IntrusiveListNode         waiter_list_node{};
            util::IntrusiveRedBlackTreeNode condvar_arbiter_tree_node{};
            util::IntrusiveListNode         process_list_node{};

            using WaiterListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&KThread::waiter_list_node>;
            using WaiterList       = WaiterListTraits::ListType;

            WaiterList                      waiter_list{};
            WaiterList                      paused_waiter_list{};
            KThread                        *lock_owner{};
            KConditionVariable             *cond_var{};
            uintptr_t                       debug_params[3]{};
            u32                             arbiter_value{};
            u32                             suspend_request_flags{};
            u32                             suspend_allowed_flags{};
            Result                          wait_result;
            Result                          debug_exception_result;
            s32                             priority{};
            s32                             core_id{};
            s32                             base_priority{};
            s32                             ideal_core_id{};
            s32                             num_kernel_waiters{};
            KAffinityMask                   original_affinity_mask{};
            s32                             original_ideal_core_id{};
            s32                             num_core_migration_disables{};
            ThreadState                     thread_state{};
            std::atomic<bool>               termination_requested{};
            bool                            ipc_cancelled{};
            bool                            wait_cancelled{};
            bool                            cancellable{};
            bool                            registered{};
            bool                            signaled{};
            bool                            initialized{};
            bool                            debug_attached{};
            s8                              priority_inheritance_count{};
            bool                            resource_limit_release_hint{};
        public:
            constexpr KThread() : wait_result(svc::ResultNoSynchronizationObject()), debug_exception_result(ResultSuccess()) { /* ... */ }

            virtual ~KThread() { /* ... */ }
            /* TODO: Is a constexpr KThread() possible? */

            Result Initialize(KThreadFunction func, uintptr_t arg, void *kern_stack_top, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner, ThreadType type);

        private:
            static Result InitializeThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner, ThreadType type);
        public:
            static Result InitializeKernelThread(KThread *thread, KThreadFunction func, uintptr_t arg, s32 prio, s32 core) {
                return InitializeThread(thread, func, arg, Null<KProcessAddress>, prio, core, nullptr, ThreadType_Kernel);
            }

            static Result InitializeHighPriorityThread(KThread *thread, KThreadFunction func, uintptr_t arg) {
                return InitializeThread(thread, func, arg, Null<KProcessAddress>, 0, GetCurrentCoreId(), nullptr, ThreadType_HighPriority);
            }

            static Result InitializeUserThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner) {
                return InitializeThread(thread, func, arg, user_stack_top, prio, core, owner, ThreadType_User);
            }

            static void ResumeThreadsSuspendedForInit();
        private:
            StackParameters &GetStackParameters() {
                return *(reinterpret_cast<StackParameters *>(this->kernel_stack_top) - 1);
            }

            const StackParameters &GetStackParameters() const {
                return *(reinterpret_cast<const StackParameters *>(this->kernel_stack_top) - 1);
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
                return this->GetDpc() != 0;;
            }
        private:
            void Suspend();
            ALWAYS_INLINE void AddWaiterImpl(KThread *thread);
            ALWAYS_INLINE void RemoveWaiterImpl(KThread *thread);
            ALWAYS_INLINE static void RestorePriority(KThread *thread);
        public:
            constexpr u64 GetThreadId() const { return this->thread_id; }

            constexpr KThreadContext &GetContext() { return this->thread_context; }
            constexpr const KThreadContext &GetContext() const { return this->thread_context; }
            constexpr const KAffinityMask &GetAffinityMask() const { return this->affinity_mask; }
            constexpr ThreadState GetState() const { return static_cast<ThreadState>(this->thread_state & ThreadState_Mask); }
            constexpr ThreadState GetRawState() const { return this->thread_state; }
            NOINLINE void SetState(ThreadState state);

            NOINLINE KThreadContext *GetContextForSchedulerLoop();

            constexpr uintptr_t GetConditionVariableKey() const { return this->condvar_key; }

            constexpr s32 GetIdealCore() const { return this->ideal_core_id; }
            constexpr s32 GetActiveCore() const { return this->core_id; }
            constexpr void SetActiveCore(s32 core) { this->core_id = core; }
            constexpr s32 GetPriority() const { return this->priority; }
            constexpr void SetPriority(s32 prio) { this->priority = prio; }
            constexpr s32 GetBasePriority() const { return this->base_priority; }

            constexpr QueueEntry &GetPriorityQueueEntry(s32 core) { return this->per_core_priority_queue_entry[core]; }
            constexpr const QueueEntry &GetPriorityQueueEntry(s32 core) const { return this->per_core_priority_queue_entry[core]; }

            constexpr QueueEntry &GetSleepingQueueEntry() { return this->sleeping_queue_entry; }
            constexpr const QueueEntry &GetSleepingQueueEntry() const { return this->sleeping_queue_entry; }
            constexpr void SetSleepingQueue(KThreadQueue *q) { this->sleeping_queue = q; }

            constexpr KConditionVariable *GetConditionVariable() const { return this->cond_var; }

            constexpr s32 GetNumKernelWaiters() const { return this->num_kernel_waiters; }

            void AddWaiter(KThread *thread);
            void RemoveWaiter(KThread *thread);
            KThread *RemoveWaiterByKey(s32 *out_num_waiters, KProcessAddress key);

            constexpr KProcessAddress GetAddressKey() const { return this->arbiter_key; }
            constexpr void SetAddressKey(KProcessAddress key) { this->arbiter_key = key; }
            constexpr void SetLockOwner(KThread *owner) { this->lock_owner = owner; }
            constexpr KThread *GetLockOwner() const { return this->lock_owner; }

            constexpr void SetSyncedObject(KSynchronizationObject *obj, Result wait_res) {
                this->synced_object = obj;
                this->wait_result = wait_res;
            }

            bool HasWaiters() const { return !this->waiter_list.empty(); }

            constexpr s64 GetLastScheduledTick() const { return this->last_scheduled_tick; }
            constexpr void SetLastScheduledTick(s64 tick) { this->last_scheduled_tick = tick; }

            constexpr KProcess *GetOwnerProcess() const { return this->parent; }
            constexpr bool IsUserThread() const { return this->parent != nullptr; }

            constexpr KProcessAddress GetThreadLocalRegionAddress() const { return this->tls_address; }
            constexpr void           *GetThreadLocalRegionHeapAddress() const { return this->tls_heap_address; }

            constexpr u16 GetUserPreemptionState() const { return *GetPointer<u16>(this->tls_address + 0x100); }
            constexpr void SetKernelPreemptionState(u16 state) const { *GetPointer<u16>(this->tls_address + 0x100 + sizeof(u16)) = state; }

            void AddCpuTime(s64 amount) {
                this->cpu_time += amount;
            }

            constexpr u32 GetSuspendFlags() const { return this->suspend_allowed_flags & this->suspend_request_flags; }
            constexpr bool IsSuspended() const { return this->GetSuspendFlags() != 0; }
            void RequestSuspend(SuspendType type);
            void Resume(SuspendType type);
            void TrySuspend();
            void Continue();

            void ContinueIfHasKernelWaiters() {
                if (this->GetNumKernelWaiters() > 0) {
                    this->Continue();
                }
            }

            void Wakeup();

            Result SetPriorityToIdle();

            Result Run();
            void Exit();

            ALWAYS_INLINE void *GetStackTop() const { return reinterpret_cast<StackParameters *>(this->kernel_stack_top) - 1; }
            ALWAYS_INLINE void *GetKernelStackTop() const { return this->kernel_stack_top; }

            /* TODO: This is kind of a placeholder definition. */

            ALWAYS_INLINE bool IsTerminationRequested() const {
                return this->termination_requested || this->GetRawState() == ThreadState_Terminated;
            }

        public:
            /* Overridden parent functions. */
            virtual u64 GetId() const override { return this->GetThreadId(); }

            virtual bool IsInitialized() const override { return this->initialized; }
            virtual uintptr_t GetPostDestroyArgument() const override { return reinterpret_cast<uintptr_t>(this->parent) | (this->resource_limit_release_hint ? 1 : 0); }

            static void PostDestroy(uintptr_t arg);

            virtual void Finalize() override;
            virtual bool IsSignaled() const override;
            virtual void OnTimer() override;
            virtual void DoWorkerTask() override;
        public:
            static constexpr bool IsWaiterListValid() {
                return WaiterListTraits::IsValid();
            }
    };
    static_assert(alignof(KThread) == 0x10);
    static_assert(KThread::IsWaiterListValid());

    class KScopedDisableDispatch {
        public:
            explicit ALWAYS_INLINE KScopedDisableDispatch() {
                GetCurrentThread().DisableDispatch();
            }

            ALWAYS_INLINE ~KScopedDisableDispatch() {
                GetCurrentThread().EnableDispatch();
            }
    };

    class KScopedEnableDispatch {
        public:
            explicit ALWAYS_INLINE KScopedEnableDispatch() {
                GetCurrentThread().EnableDispatch();
            }

            ALWAYS_INLINE ~KScopedEnableDispatch() {
                GetCurrentThread().DisableDispatch();
            }
    };

}
