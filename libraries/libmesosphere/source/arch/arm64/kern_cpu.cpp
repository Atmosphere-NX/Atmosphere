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

namespace ams::kern::arch::arm64::cpu {

    /* Declare prototype to be implemented in asm. */
    void SynchronizeAllCoresImpl(s32 *sync_var, s32 num_cores);


    namespace {

        class KScopedCoreMigrationDisable {
            public:
                ALWAYS_INLINE KScopedCoreMigrationDisable() { GetCurrentThread().DisableCoreMigration(); }

                ALWAYS_INLINE ~KScopedCoreMigrationDisable() { GetCurrentThread().EnableCoreMigration(); }
        };

        /* Nintendo registers a handler for a SGI on thread termination, but does not handle anything. */
        /* This is sufficient, because post-interrupt scheduling is all they really intend to occur. */
        class KThreadTerminationInterruptHandler : public KInterruptHandler {
            public:
                constexpr KThreadTerminationInterruptHandler() : KInterruptHandler() { /* ... */ }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);
                    return nullptr;
                }
        };

        class KPerformanceCounterInterruptHandler : public KInterruptHandler {
            private:
                static inline KLightLock s_lock;
            private:
                u64 counter;
                s32 which;
                bool done;
            public:
                constexpr KPerformanceCounterInterruptHandler() : KInterruptHandler(), counter(), which(), done() { /* ... */ }

                static KLightLock &GetLock() { return s_lock; }

                void Setup(s32 w) {
                    this->done = false;
                    this->which = w;
                }

                void Wait() {
                    while (!this->done) {
                        cpu::Yield();
                    }
                }

                u64 GetCounter() const { return this->counter; }

                /* Nintendo misuses this per their own API, but it's functional. */
                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);

                    if (this->which < 0) {
                        this->counter = cpu::GetCycleCounter();
                    } else {
                        this->counter = cpu::GetPerformanceCounter(this->which);
                    }
                    DataMemoryBarrier();
                    this->done = true;
                    return nullptr;
                }
        };

        class KCacheHelperInterruptHandler : public KInterruptHandler {
            private:
                static constexpr s32 ThreadPriority = 8;
            public:
                enum class Operation {
                    Idle,
                    InstructionMemoryBarrier,
                    StoreDataCache,
                    FlushDataCache,
                };
            private:
                KLightLock lock;
                KLightLock cv_lock;
                KLightConditionVariable cv;
                std::atomic<u64> target_cores;
                volatile Operation operation;
            private:
                static void ThreadFunction(uintptr_t _this) {
                    reinterpret_cast<KCacheHelperInterruptHandler *>(_this)->ThreadFunctionImpl();
                }

                void ThreadFunctionImpl() {
                    const s32 core_id = GetCurrentCoreId();
                    while (true) {
                        /* Wait for a request to come in. */
                        {
                            KScopedLightLock lk(this->cv_lock);
                            while ((this->target_cores & (1ul << core_id)) == 0) {
                                this->cv.Wait(std::addressof(this->cv_lock));
                            }
                        }

                        /* Process the request. */
                        this->ProcessOperation();

                        /* Broadcast, if there's nothing pending. */
                        {
                            KScopedLightLock lk(this->cv_lock);
                            if (this->target_cores == 0) {
                                this->cv.Broadcast();
                            }
                        }
                    }
                }

                void ProcessOperation();
            public:
                constexpr KCacheHelperInterruptHandler() : KInterruptHandler(), lock(), cv_lock(), cv(), target_cores(), operation(Operation::Idle) { /* ... */ }

                void Initialize(s32 core_id) {
                    /* Reserve a thread from the system limit. */
                    MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

                    /* Create a new thread. */
                    KThread *new_thread = KThread::Create();
                    MESOSPHERE_ABORT_UNLESS(new_thread != nullptr);
                    MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(new_thread, ThreadFunction, reinterpret_cast<uintptr_t>(this), ThreadPriority, core_id));

                    /* Register the new thread. */
                    KThread::Register(new_thread);

                    /* Run the thread. */
                    new_thread->Run();
                }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);
                    this->ProcessOperation();
                    return nullptr;
                }

                void RequestOperation(Operation op) {
                    KScopedLightLock lk(this->lock);

                    /* Create core masks for us to use. */
                    constexpr u64 AllCoresMask = (1ul << cpu::NumCores) - 1ul;
                    const u64 other_cores_mask = AllCoresMask & ~(1ul << GetCurrentCoreId());

                    if ((op == Operation::InstructionMemoryBarrier) || (Kernel::GetState() == Kernel::State::Initializing)) {
                        /* Check that there's no on-going operation. */
                        MESOSPHERE_ABORT_UNLESS(this->operation == Operation::Idle);
                        MESOSPHERE_ABORT_UNLESS(this->target_cores == 0);

                        /* Set operation. */
                        this->operation = op;

                        /* For certain operations, we want to send an interrupt. */
                        this->target_cores = other_cores_mask;

                        const u64 target_mask = this->target_cores;
                        DataSynchronizationBarrier();
                        Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_CacheOperation, target_mask);

                        this->ProcessOperation();
                        while (this->target_cores != 0) {
                            cpu::Yield();
                        }

                        /* Go idle again. */
                        this->operation = Operation::Idle;
                    } else {
                        /* Lock condvar so that we can send and wait for acknowledgement of request. */
                        KScopedLightLock cv_lk(this->cv_lock);

                        /* Check that there's no on-going operation. */
                        MESOSPHERE_ABORT_UNLESS(this->operation == Operation::Idle);
                        MESOSPHERE_ABORT_UNLESS(this->target_cores == 0);

                        /* Set operation. */
                        this->operation = op;

                        /* Request all cores. */
                        this->target_cores = AllCoresMask;

                        /* Use the condvar. */
                        this->cv.Broadcast();
                        while (this->target_cores != 0) {
                            this->cv.Wait(std::addressof(this->cv_lock));
                        }

                        /* Go idle again. */
                        this->operation = Operation::Idle;
                    }
                }
        };

        /* Instances of the interrupt handlers. */
        KThreadTerminationInterruptHandler  g_thread_termination_handler;
        KCacheHelperInterruptHandler        g_cache_operation_handler;
        KPerformanceCounterInterruptHandler g_performance_counter_handler[cpu::NumCores];

        /* Expose this as a global, for asm to use. */
        s32 g_all_core_sync_count;

        template<bool Init, typename F>
        ALWAYS_INLINE void PerformCacheOperationBySetWayImpl(int level, F f) {
            /* Used in multiple locations. */
            const u64 level_sel_value = static_cast<u64>(level << 1);

            u64 ccsidr_value;
            if constexpr (Init) {
                /* During init, we can just set the selection register directly. */
                cpu::SetCsselrEl1(level_sel_value);
                cpu::InstructionMemoryBarrier();
                ccsidr_value = cpu::GetCcsidrEl1();
            } else {
                /* After init, we need to care about interrupts. */
                KScopedInterruptDisable di;
                cpu::SetCsselrEl1(level_sel_value);
                cpu::InstructionMemoryBarrier();
                ccsidr_value = cpu::GetCcsidrEl1();
            }

            /* Get cache size id info. */
            CacheSizeIdRegisterAccessor ccsidr_el1(ccsidr_value);
            const int num_sets  = ccsidr_el1.GetNumberOfSets();
            const int num_ways  = ccsidr_el1.GetAssociativity();
            const int line_size = ccsidr_el1.GetLineSize();

            const u64 way_shift = static_cast<u64>(__builtin_clz(num_ways));
            const u64 set_shift = static_cast<u64>(line_size + 4);

            for (int way = 0; way <= num_ways; way++) {
                for (int set = 0; set <= num_sets; set++) {
                    const u64 way_value  = static_cast<u64>(way) << way_shift;
                    const u64 set_value  = static_cast<u64>(set) << set_shift;
                    f(way_value | set_value | level_sel_value);
                }
            }
        }

        ALWAYS_INLINE void FlushDataCacheLineBySetWayImpl(const u64 sw_value) {
            __asm__ __volatile__("dc cisw, %[v]" :: [v]"r"(sw_value) : "memory");
        }

        ALWAYS_INLINE void StoreDataCacheLineBySetWayImpl(const u64 sw_value) {
            __asm__ __volatile__("dc csw, %[v]" :: [v]"r"(sw_value) : "memory");
        }

        template<bool Init, typename F>
        ALWAYS_INLINE void PerformCacheOperationBySetWayShared(F f) {
            CacheLineIdRegisterAccessor clidr_el1;
            const int levels_of_coherency   = clidr_el1.GetLevelsOfCoherency();
            const int levels_of_unification = clidr_el1.GetLevelsOfUnification();

            for (int level = levels_of_coherency; level >= levels_of_unification; level--) {
                PerformCacheOperationBySetWayImpl<Init>(level, f);
            }
        }

        template<bool Init, typename F>
        ALWAYS_INLINE void PerformCacheOperationBySetWayLocal(F f) {
            CacheLineIdRegisterAccessor clidr_el1;
            const int levels_of_unification = clidr_el1.GetLevelsOfUnification();

            for (int level = levels_of_unification - 1; level >= 0; level--) {
                PerformCacheOperationBySetWayImpl<Init>(level, f);
            }
        }

        void KCacheHelperInterruptHandler::ProcessOperation() {
            switch (this->operation) {
                case Operation::Idle:
                    break;
                case Operation::InstructionMemoryBarrier:
                    InstructionMemoryBarrier();
                    break;
                case Operation::StoreDataCache:
                    PerformCacheOperationBySetWayLocal<false>(StoreDataCacheLineBySetWayImpl);
                    DataSynchronizationBarrier();
                    break;
                case Operation::FlushDataCache:
                    PerformCacheOperationBySetWayLocal<false>(FlushDataCacheLineBySetWayImpl);
                    DataSynchronizationBarrier();
                    break;
            }

            this->target_cores &= ~(1ul << GetCurrentCoreId());
        }

        ALWAYS_INLINE void SetEventLocally() {
            __asm__ __volatile__("sevl" ::: "memory");
        }

        ALWAYS_INLINE void WaitForEvent() {
            __asm__ __volatile__("wfe" ::: "memory");
        }

        ALWAYS_INLINE Result InvalidateDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::InvalidateDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result StoreDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::StoreDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result FlushDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::FlushDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result InvalidateInstructionCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, InstructionCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   InstructionCacheLineSize));
            R_UNLESS(UserspaceAccess::InvalidateInstructionCache(start, end), svc::ResultInvalidCurrentMemory());
            EnsureInstructionConsistency();
            return ResultSuccess();
        }

        ALWAYS_INLINE void InvalidateEntireInstructionCacheLocalImpl() {
            __asm__ __volatile__("ic iallu" ::: "memory");
        }

        ALWAYS_INLINE void InvalidateEntireInstructionCacheGlobalImpl() {
            __asm__ __volatile__("ic ialluis" ::: "memory");
        }

    }

    void FlushEntireDataCacheSharedForInit() {
        return PerformCacheOperationBySetWayShared<true>(FlushDataCacheLineBySetWayImpl);
    }

    void FlushEntireDataCacheLocalForInit() {
        return PerformCacheOperationBySetWayLocal<true>(FlushDataCacheLineBySetWayImpl);
    }

    void InvalidateEntireInstructionCacheForInit() {
        InvalidateEntireInstructionCacheLocalImpl();
        EnsureInstructionConsistency();
    }

    void StoreEntireCacheForInit() {
        PerformCacheOperationBySetWayLocal<true>(StoreDataCacheLineBySetWayImpl);
        PerformCacheOperationBySetWayShared<true>(StoreDataCacheLineBySetWayImpl);
        DataSynchronizationBarrierInnerShareable();
        InvalidateEntireInstructionCacheForInit();
    }

    void FlushEntireDataCache() {
        return PerformCacheOperationBySetWayShared<false>(FlushDataCacheLineBySetWayImpl);
    }

    Result InvalidateDataCache(void *addr, size_t size) {
        KScopedCoreMigrationDisable dm;
        const uintptr_t start = reinterpret_cast<uintptr_t>(addr);
        const uintptr_t end   = start + size;
        uintptr_t aligned_start = util::AlignDown(start, DataCacheLineSize);
        uintptr_t aligned_end   = util::AlignUp(end, DataCacheLineSize);

        if (aligned_start != start) {
            R_TRY(FlushDataCacheRange(aligned_start, aligned_start + DataCacheLineSize));
            aligned_start += DataCacheLineSize;
        }

        if (aligned_start < aligned_end && (aligned_end != end)) {
            aligned_end -= DataCacheLineSize;
            R_TRY(FlushDataCacheRange(aligned_end, aligned_end + DataCacheLineSize));
        }

        if (aligned_start < aligned_end) {
            R_TRY(InvalidateDataCacheRange(aligned_start, aligned_end));
        }

        return ResultSuccess();
    }

    Result StoreDataCache(const void *addr, size_t size) {
        KScopedCoreMigrationDisable dm;
        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr),        DataCacheLineSize);
        const uintptr_t end   = util::AlignUp(  reinterpret_cast<uintptr_t>(addr) + size, DataCacheLineSize);

        return StoreDataCacheRange(start, end);
    }

    Result FlushDataCache(const void *addr, size_t size) {
        KScopedCoreMigrationDisable dm;
        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr),        DataCacheLineSize);
        const uintptr_t end   = util::AlignUp(  reinterpret_cast<uintptr_t>(addr) + size, DataCacheLineSize);

        return FlushDataCacheRange(start, end);
    }

    Result InvalidateInstructionCache(void *addr, size_t size) {
        KScopedCoreMigrationDisable dm;
        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr),        InstructionCacheLineSize);
        const uintptr_t end   = util::AlignUp(  reinterpret_cast<uintptr_t>(addr) + size, InstructionCacheLineSize);

        R_TRY(InvalidateInstructionCacheRange(start, end));

        /* Request the interrupt helper to perform an instruction memory barrier. */
        g_cache_operation_handler.RequestOperation(KCacheHelperInterruptHandler::Operation::InstructionMemoryBarrier);

        return ResultSuccess();
    }

    void InvalidateEntireInstructionCache() {
        KScopedCoreMigrationDisable dm;

        /* Invalidate the instruction cache on all cores. */
        InvalidateEntireInstructionCacheGlobalImpl();
        EnsureInstructionConsistency();

        /* Request the interrupt helper to perform an instruction memory barrier. */
        g_cache_operation_handler.RequestOperation(KCacheHelperInterruptHandler::Operation::InstructionMemoryBarrier);
    }

    void InitializeInterruptThreads(s32 core_id) {
        /* Initialize the cache operation handler. */
        g_cache_operation_handler.Initialize(core_id);

        /* Bind all handlers to the relevant interrupts. */
        Kernel::GetInterruptManager().BindHandler(std::addressof(g_cache_operation_handler),              KInterruptName_CacheOperation,     core_id, KInterruptController::PriorityLevel_High,      false, false);
        Kernel::GetInterruptManager().BindHandler(std::addressof(g_thread_termination_handler),           KInterruptName_ThreadTerminate,    core_id, KInterruptController::PriorityLevel_Scheduler, false, false);

        if (KTargetSystem::IsUserPmuAccessEnabled()) { SetPmUserEnrEl0(1ul); }
        Kernel::GetInterruptManager().BindHandler(std::addressof(g_performance_counter_handler[core_id]), KInterruptName_PerformanceCounter, core_id, KInterruptController::PriorityLevel_Timer,     false, false);
    }

    void SynchronizeAllCores() {
        SynchronizeAllCoresImpl(&g_all_core_sync_count, static_cast<s32>(cpu::NumCores));
    }

}
