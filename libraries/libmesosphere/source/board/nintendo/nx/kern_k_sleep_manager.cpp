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
#include "kern_k_sleep_manager.hpp"
#include "kern_secure_monitor.hpp"

namespace ams::kern::board::nintendo::nx {

    namespace {

        /* Struct representing registers saved on wake/sleep. */
        class SavedSystemRegisters {
            private:
                u64 ttbr0_el1;
                u64 tcr_el1;
                u64 elr_el1;
                u64 sp_el0;
                u64 spsr_el1;
                u64 daif;
                u64 cpacr_el1;
                u64 vbar_el1;
                u64 csselr_el1;
                u64 cntp_ctl_el0;
                u64 cntp_cval_el0;
                u64 cntkctl_el1;
                u64 tpidr_el0;
                u64 tpidrro_el0;
                u64 mdscr_el1;
                u64 contextidr_el1;
                u64 dbgwcrN_el1[16];
                u64 dbgwvrN_el1[16];
                u64 dbgbcrN_el1[16];
                u64 dbgbvrN_el1[16];
                u64 pmccfiltr_el0;
                u64 pmccntr_el0;
                u64 pmcntenset_el0;
                u64 pmcr_el0;
                u64 pmevcntrN_el0[31];
                u64 pmevtyperN_el0[31];
                u64 pmcntenset_el1;
                u64 pmovsset_el0;
                u64 pmselr_el0;
                u64 pmuserenr_el0;
            public:
                void Save();
                void Restore() const;
        };

        constexpr s32 SleepManagerThreadPriority = 2;

        /* Globals for sleep/wake. */
        u64 g_sleep_target_cores;
        KLightLock g_request_lock;
        KLightLock g_cv_lock;
        KLightConditionVariable g_cv;
        KPhysicalAddress g_sleep_buffer_phys_addrs[cpu::NumCores];
        alignas(16) u64 g_sleep_buffers[cpu::NumCores][1_KB / sizeof(u64)];
        SavedSystemRegisters g_sleep_system_registers[cpu::NumCores] = {};

    }

    void KSleepManager::Initialize() {
        /* Create a sleep manager thread for each core. */
        for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
            /* Reserve a thread from the system limit. */
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_ThreadCountMax, 1));

            /* Create a new thread. */
            KThread *new_thread = KThread::Create();
            MESOSPHERE_ABORT_UNLESS(new_thread != nullptr);

            /* Launch the new thread. */
            MESOSPHERE_R_ABORT_UNLESS(KThread::InitializeKernelThread(new_thread, KSleepManager::ProcessRequests, reinterpret_cast<uintptr_t>(g_sleep_buffers[core_id]), SleepManagerThreadPriority, static_cast<s32>(core_id)));

            /* Register the new thread. */
            KThread::Register(new_thread);

            /* Run the thread. */
            new_thread->Run();
        }
    }

    void KSleepManager::SleepSystem() {
        /* Ensure device mappings are not modified during sleep. */
        MESOSPHERE_TODO("KDevicePageTable::Lock();");
        ON_SCOPE_EXIT { MESOSPHERE_TODO("KDevicePageTable::Unlock();"); };

        /* Request that the system sleep. */
        {
            KScopedLightLock lk(g_request_lock);

            /* Signal the manager to sleep on all cores. */
            {
                KScopedLightLock lk(g_cv_lock);
                MESOSPHERE_ABORT_UNLESS(g_sleep_target_cores == 0);

                g_sleep_target_cores = (1ul << (cpu::NumCores - 1));
                g_cv.Broadcast();

                while (g_sleep_target_cores != 0) {
                    g_cv.Wait(std::addressof(g_cv_lock));
                }
            }
        }
    }

    void KSleepManager::ProcessRequests(uintptr_t buffer) {
        const s32 core_id = GetCurrentCoreId();
        KPhysicalAddress resume_entry_phys_addr = Null<KPhysicalAddress>;

        /* Get the physical addresses we'll need. */
        {
            MESOSPHERE_ABORT_UNLESS(Kernel::GetKernelPageTable().GetPhysicalAddress(std::addressof(g_sleep_buffer_phys_addrs[core_id]), KProcessAddress(buffer)));
            MESOSPHERE_ABORT_UNLESS(Kernel::GetKernelPageTable().GetPhysicalAddress(std::addressof(resume_entry_phys_addr), KProcessAddress(&::ams::kern::board::nintendo::nx::KSleepManager::ResumeEntry)));

        }
        const KPhysicalAddress sleep_buffer_phys_addr = g_sleep_buffer_phys_addrs[core_id];
        const u64 target_core_mask = (1ul << core_id);

        /* Loop, processing sleep when requested. */
        while (true) {
            /* Wait for a request. */
            {
                KScopedLightLock lk(g_cv_lock);
                while (!(g_sleep_target_cores & target_core_mask)) {
                    g_cv.Wait(std::addressof(g_cv_lock));
                }
            }

            MESOSPHERE_TODO("Implement Sleep/Wake");
            (void)(g_sleep_system_registers[core_id]);
            (void)(sleep_buffer_phys_addr);

            /* Signal request completed. */
            {
                KScopedLightLock lk(g_cv_lock);
                g_sleep_target_cores &= ~target_core_mask;
                if (g_sleep_target_cores == 0) {
                    g_cv.Broadcast();
                }
            }
        }
    }

}
