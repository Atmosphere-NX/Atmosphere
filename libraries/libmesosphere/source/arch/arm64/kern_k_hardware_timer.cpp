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

namespace ams::kern::arch::arm64 {

    namespace impl {

        class KHardwareTimerInterruptTask : public KInterruptTask {
            public:
                constexpr KHardwareTimerInterruptTask() : KInterruptTask() { /* ... */ }

                virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                    return this;
                }

                virtual void DoTask() override {
                    Kernel::GetHardwareTimer().DoInterruptTask();
                }
        };

    }

    namespace {

        /* One global hardware timer interrupt task per core. */
        impl::KHardwareTimerInterruptTask g_hardware_timer_interrupt_tasks[cpu::NumCores];

        ALWAYS_INLINE auto *GetHardwareTimerInterruptTask(s32 core_id) {
            return std::addressof(g_hardware_timer_interrupt_tasks[core_id]);
        }

    }

    void KHardwareTimer::Initialize(s32 core_id) {
        /* Setup the global timer for the core. */
        InitializeGlobalTimer();

        /* Bind the interrupt task for this core. */
        Kernel::GetInterruptManager().BindHandler(GetHardwareTimerInterruptTask(core_id), KInterruptName_NonSecurePhysicalTimer, core_id, KInterruptController::PriorityLevel_Timer, true, true);
    }

    void KHardwareTimer::Finalize() {
        /* Stop the hardware timer. */
        StopTimer();
    }

    void KHardwareTimer::DoInterruptTask() {
        /* Handle the interrupt. */
        {
            KScopedSchedulerLock slk;
            KScopedSpinLock lk(this->GetLock());

            /* Disable the timer interrupt while we handle this. */
            DisableInterrupt();
            if (const s64 next_time = this->DoInterruptTaskImpl(GetTick()); next_time > 0) {
                /* We have a next time, so we should set the time to interrupt and turn the interrupt on. */
                SetCompareValue(next_time);
                EnableInterrupt();
            }
        }

        /* Clear the timer interrupt. */
        Kernel::GetInterruptManager().ClearInterrupt(KInterruptName_NonSecurePhysicalTimer, GetCurrentCoreId());
    }

}
