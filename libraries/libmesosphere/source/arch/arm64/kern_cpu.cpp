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

namespace ams::kern::arch::arm64::cpu {

    /* Declare prototype to be implemented in asm. */
    void SynchronizeAllCoresImpl(s32 *sync_var, s32 num_cores);


    namespace {

        ALWAYS_INLINE void SetEventLocally() {
            __asm__ __volatile__("sevl" ::: "memory");
        }

        ALWAYS_INLINE void WaitForEvent() {
            __asm__ __volatile__("wfe" ::: "memory");
        }

        class KScopedCoreMigrationDisable {
            public:
                ALWAYS_INLINE KScopedCoreMigrationDisable() { GetCurrentThread().DisableCoreMigration(); }

                ALWAYS_INLINE ~KScopedCoreMigrationDisable() { GetCurrentThread().EnableCoreMigration(); }
        };

        class KScopedCacheMaintenance {
            private:
                bool m_active;
            public:
                ALWAYS_INLINE KScopedCacheMaintenance() {
                    __asm__ __volatile__("" ::: "memory");
                    if (m_active = !GetCurrentThread().IsInCacheMaintenanceOperation(); m_active) {
                        GetCurrentThread().SetInCacheMaintenanceOperation();
                    }
                }

                ALWAYS_INLINE ~KScopedCacheMaintenance() {
                    if (m_active) {
                        GetCurrentThread().ClearInCacheMaintenanceOperation();
                    }
                    __asm__ __volatile__("" ::: "memory");
                }
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
                static constinit inline KLightLock s_lock;
            private:
                u64 m_counter;
                s32 m_which;
                bool m_done;
            public:
                constexpr KPerformanceCounterInterruptHandler() : KInterruptHandler(), m_counter(), m_which(), m_done() { /* ... */ }

                static KLightLock &GetLock() { return s_lock; }

                void Setup(s32 w) {
                    m_done = false;
                    m_which = w;
                }

                void Wait() {
                    while (!m_done) {
                        cpu::Yield();
                    }
                }

                u64 GetCounter() const { return m_counter; }

                /* Nintendo misuses this per their own API, but it's functional. */
                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);

                    if (m_which < 0) {
                        m_counter = cpu::GetCycleCounter();
                    } else {
                        m_counter = cpu::GetPerformanceCounter(m_which);
                    }
                    DataMemoryBarrierInnerShareable();
                    m_done = true;
                    return nullptr;
                }
        };

        class KCoreBarrierInterruptHandler : public KInterruptHandler {
            private:
                util::Atomic<u64> m_target_cores;
                KLightLock m_lock;
            public:
                constexpr KCoreBarrierInterruptHandler() : KInterruptHandler(), m_target_cores(0), m_lock() { /* ... */ }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    MESOSPHERE_UNUSED(interrupt_id);
                    m_target_cores &= ~(1ul << GetCurrentCoreId());
                    return nullptr;
                }

                void SynchronizeCores(u64 core_mask) {
                    /* Acquire exclusive access to ourselves. */
                    KScopedLightLock lk(m_lock);

                    /* If necessary, force synchronization with other cores. */
                    if (const u64 other_cores_mask = core_mask & ~(1ul << GetCurrentCoreId()); other_cores_mask != 0) {
                        /* Send an interrupt to the other cores. */
                        m_target_cores = other_cores_mask;
                        cpu::DataSynchronizationBarrierInnerShareable();
                        Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_CoreBarrier, other_cores_mask);

                        /* Wait for all cores to acknowledge. */
                        {
                            u64 v;
                            __asm__ __volatile__("ldaxr %[v], %[p]\n"
                                                 "cbz %[v], 1f\n"
                                                 "0:\n"
                                                 "wfe\n"
                                                 "ldaxr %[v], %[p]\n"
                                                 "cbnz %[v], 0b\n"
                                                 "1:\n"
                                                 : [v]"=&r"(v)
                                                 : [p]"Q"(*reinterpret_cast<u64 *>(std::addressof(m_target_cores)))
                                                 : "memory");
                        }
                    }
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
                KLightLock m_lock;
                KLightLock m_cv_lock;
                KLightConditionVariable m_cv;
                util::Atomic<u64> m_target_cores;
                volatile Operation m_operation;
            private:
                static void ThreadFunction(uintptr_t _this) {
                    reinterpret_cast<KCacheHelperInterruptHandler *>(_this)->ThreadFunctionImpl();
                }

                void ThreadFunctionImpl() {
                    const u64 core_mask = (1ul << GetCurrentCoreId());
                    while (true) {
                        /* Wait for a request to come in. */
                        {
                            KScopedLightLock lk(m_cv_lock);
                            while ((m_target_cores.Load() & core_mask) == 0) {
                                m_cv.Wait(std::addressof(m_cv_lock));
                            }
                        }

                        /* Process the request. */
                        this->ProcessOperation();

                        /* Broadcast, if there's nothing pending. */
                        {
                            KScopedLightLock lk(m_cv_lock);

                            m_target_cores &= ~core_mask;
                            if (m_target_cores.Load() == 0) {
                                m_cv.Broadcast();
                            }
                        }
                    }
                }

                void ProcessOperation();
            public:
                constexpr KCacheHelperInterruptHandler() : KInterruptHandler(), m_lock(), m_cv_lock(), m_cv(util::ConstantInitialize), m_target_cores(0), m_operation(Operation::Idle) { /* ... */ }

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
                    m_target_cores &= ~(1ul << GetCurrentCoreId());
                    return nullptr;
                }

                void RequestOperation(Operation op) {
                    KScopedLightLock lk(m_lock);

                    /* Create core masks for us to use. */
                    constexpr u64 AllCoresMask = (1ul << cpu::NumCores) - 1ul;
                    const u64 other_cores_mask = AllCoresMask & ~(1ul << GetCurrentCoreId());

                    if ((op == Operation::InstructionMemoryBarrier) || (Kernel::GetState() == Kernel::State::Initializing)) {
                        /* Check that there's no on-going operation. */
                        MESOSPHERE_ABORT_UNLESS(m_operation == Operation::Idle);
                        MESOSPHERE_ABORT_UNLESS(m_target_cores.Load() == 0);

                        /* Set operation. */
                        m_operation = op;

                        /* For certain operations, we want to send an interrupt. */
                        m_target_cores = other_cores_mask;

                        const u64 target_mask = m_target_cores.Load();

                        DataSynchronizationBarrierInnerShareable();
                        Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_CacheOperation, target_mask);

                        this->ProcessOperation();
                        while (m_target_cores.Load() != 0) {
                            cpu::Yield();
                        }

                        /* Go idle again. */
                        m_operation = Operation::Idle;
                    } else {
                        /* Lock condvar so that we can send and wait for acknowledgement of request. */
                        KScopedLightLock cv_lk(m_cv_lock);

                        /* Check that there's no on-going operation. */
                        MESOSPHERE_ABORT_UNLESS(m_operation == Operation::Idle);
                        MESOSPHERE_ABORT_UNLESS(m_target_cores.Load() == 0);

                        /* Set operation. */
                        m_operation = op;

                        /* Request all cores. */
                        m_target_cores = AllCoresMask;

                        /* Use the condvar. */
                        m_cv.Broadcast();
                        while (m_target_cores.Load() != 0) {
                            m_cv.Wait(std::addressof(m_cv_lock));
                        }

                        /* Go idle again. */
                        m_operation = Operation::Idle;
                    }
                }
        };

        /* Instances of the interrupt handlers. */
        constinit KThreadTerminationInterruptHandler  g_thread_termination_handler;
        constinit KCacheHelperInterruptHandler        g_cache_operation_handler;
        constinit KCoreBarrierInterruptHandler        g_core_barrier_handler;

        #if defined(MESOSPHERE_ENABLE_PERFORMANCE_COUNTER)
        constinit KPerformanceCounterInterruptHandler g_performance_counter_handler[cpu::NumCores];
        #endif

        /* Expose this as a global, for asm to use. */
        constinit s32 g_all_core_sync_count;

        template<typename F>
        ALWAYS_INLINE void PerformCacheOperationBySetWayImpl(int level, F f) {
            /* Used in multiple locations. */
            const u64 level_sel_value = static_cast<u64>(level << 1);

            /* Get the cache size id register value with interrupts disabled. */
            u64 ccsidr_value;
            {
                /* Disable interrupts. */
                KScopedInterruptDisable di;

                /* Configure the cache select register for our level. */
                cpu::SetCsselrEl1(level_sel_value);

                /* Ensure our configuration takes before reading the cache size id register. */
                cpu::InstructionMemoryBarrier();

                /* Get the cache size id register. */
                ccsidr_value = cpu::GetCcsidrEl1();
            }

            /* Ensure that no memory inconsistencies occur between cache management invocations. */
            cpu::DataSynchronizationBarrier();

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

        void StoreDataCacheBySetWay(int level) {
            PerformCacheOperationBySetWayImpl(level, StoreDataCacheLineBySetWayImpl);
        }

        void FlushDataCacheBySetWay(int level) {
            PerformCacheOperationBySetWayImpl(level, FlushDataCacheLineBySetWayImpl);
        }

        void KCacheHelperInterruptHandler::ProcessOperation() {
            switch (m_operation) {
                case Operation::Idle:
                    break;
                case Operation::InstructionMemoryBarrier:
                    InstructionMemoryBarrier();
                    break;
                case Operation::StoreDataCache:
                    StoreDataCacheBySetWay(0);
                    cpu::DataSynchronizationBarrier();
                    break;
                case Operation::FlushDataCache:
                    FlushDataCacheBySetWay(0);
                    cpu::DataSynchronizationBarrier();
                    break;
            }
        }

        ALWAYS_INLINE Result InvalidateDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::InvalidateDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            R_SUCCEED();
        }

        ALWAYS_INLINE Result StoreDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::StoreDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            R_SUCCEED();
        }

        ALWAYS_INLINE Result FlushDataCacheRange(uintptr_t start, uintptr_t end) {
            MESOSPHERE_ASSERT(util::IsAligned(start, DataCacheLineSize));
            MESOSPHERE_ASSERT(util::IsAligned(end,   DataCacheLineSize));
            R_UNLESS(UserspaceAccess::FlushDataCache(start, end), svc::ResultInvalidCurrentMemory());
            DataSynchronizationBarrier();
            R_SUCCEED();
        }

        ALWAYS_INLINE void InvalidateEntireInstructionCacheLocalImpl() {
            __asm__ __volatile__("ic iallu" ::: "memory");
        }

        ALWAYS_INLINE void InvalidateEntireInstructionCacheGlobalImpl() {
            __asm__ __volatile__("ic ialluis" ::: "memory");
        }

    }

    void SynchronizeCores(u64 core_mask) {
        /* Request a core barrier interrupt. */
        g_core_barrier_handler.SynchronizeCores(core_mask);
    }

    void StoreCacheForInit(void *addr, size_t size) {
        /* Store the data cache for the specified range. */
        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr), DataCacheLineSize);
        const uintptr_t end   = start + size;
        for (uintptr_t cur = start; cur < end; cur += DataCacheLineSize) {
            __asm__ __volatile__("dc cvac, %[cur]" :: [cur]"r"(cur) : "memory");
        }

        /* Data synchronization barrier. */
        DataSynchronizationBarrierInnerShareable();

        /* Invalidate instruction cache. */
        InvalidateEntireInstructionCacheLocalImpl();

        /* Ensure local instruction consistency. */
        EnsureInstructionConsistency();
    }

    void FlushEntireDataCache() {
        KScopedCoreMigrationDisable dm;

        CacheLineIdRegisterAccessor clidr_el1;
        const int levels_of_coherency   = clidr_el1.GetLevelsOfCoherency();

        /* Store cache from L2 up to the level of coherence (if there's an L3 cache or greater). */
        for (int level = 2; level < levels_of_coherency; ++level) {
            StoreDataCacheBySetWay(level - 1);
        }

        /* Flush cache from the level of coherence down to L2. */
        for (int level = levels_of_coherency; level > 1; --level) {
            FlushDataCacheBySetWay(level - 1);
        }

        /* Data synchronization barrier for full system. */
        DataSynchronizationBarrier();
    }

    Result InvalidateDataCache(void *addr, size_t size) {
        /* Mark ourselves as in a cache maintenance operation, and prevent re-ordering. */
        KScopedCacheMaintenance cm;

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

        R_SUCCEED();
    }

    Result StoreDataCache(const void *addr, size_t size) {
        /* Mark ourselves as in a cache maintenance operation, and prevent re-ordering. */
        KScopedCacheMaintenance cm;

        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr),        DataCacheLineSize);
        const uintptr_t end   = util::AlignUp(  reinterpret_cast<uintptr_t>(addr) + size, DataCacheLineSize);

        R_RETURN(StoreDataCacheRange(start, end));
    }

    Result FlushDataCache(const void *addr, size_t size) {
        /* Mark ourselves as in a cache maintenance operation, and prevent re-ordering. */
        KScopedCacheMaintenance cm;

        const uintptr_t start = util::AlignDown(reinterpret_cast<uintptr_t>(addr),        DataCacheLineSize);
        const uintptr_t end   = util::AlignUp(  reinterpret_cast<uintptr_t>(addr) + size, DataCacheLineSize);

        R_RETURN(FlushDataCacheRange(start, end));
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
        Kernel::GetInterruptManager().BindHandler(std::addressof(g_core_barrier_handler),                 KInterruptName_CoreBarrier,        core_id, KInterruptController::PriorityLevel_Scheduler, false, false);

        /* If we should, enable user access to the performance counter registers. */
        if (KTargetSystem::IsUserPmuAccessEnabled()) { SetPmUserEnrEl0(1ul); }

        /* If we should, enable the kernel performance counter interrupt handler. */
        #if defined(MESOSPHERE_ENABLE_PERFORMANCE_COUNTER)
        Kernel::GetInterruptManager().BindHandler(std::addressof(g_performance_counter_handler[core_id]), KInterruptName_PerformanceCounter, core_id, KInterruptController::PriorityLevel_Timer,     false, false);
        #endif
    }

    void SynchronizeAllCores() {
        SynchronizeAllCoresImpl(&g_all_core_sync_count, static_cast<s32>(cpu::NumCores));
    }

}
