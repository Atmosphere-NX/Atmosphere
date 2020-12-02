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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_hardware_timer_base.hpp>

namespace ams::kern::arch::arm64 {

    class KHardwareTimer : public KInterruptTask, public KHardwareTimerBase {
        private:
            s64 maximum_time;
        public:
            constexpr KHardwareTimer() : KInterruptTask(), KHardwareTimerBase(), maximum_time(std::numeric_limits<s64>::max()) { /* ... */ }
        public:
            /* Public API. */
            NOINLINE void Initialize();
            NOINLINE void Finalize();

            static s64 GetTick() {
                return GetCount();
            }

            void RegisterAbsoluteTask(KTimerTask *task, s64 task_time) {
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(this->GetLock());

                if (this->RegisterAbsoluteTaskImpl(task, task_time)) {
                    if (task_time <= this->maximum_time) {
                        SetCompareValue(task_time);
                        EnableInterrupt();
                    }
                }
            }
        private:
            /* Hardware register accessors. */
            static ALWAYS_INLINE void InitializeGlobalTimer() {
                /* Set kernel control. */
                cpu::CounterTimerKernelControlRegisterAccessor(0).SetEl0PctEn(true).Store();

                /* Disable the physical timer. */
                cpu::CounterTimerPhysicalTimerControlRegisterAccessor(0).SetEnable(false).SetIMask(false).Store();

                /* Set the compare value to the maximum. */
                cpu::CounterTimerPhysicalTimerCompareValueRegisterAccessor(0).SetCompareValue(std::numeric_limits<u64>::max()).Store();

                /* Enable the physical timer, with interrupt masked. */
                cpu::CounterTimerPhysicalTimerControlRegisterAccessor(0).SetEnable(true).SetIMask(true).Store();
            }

            static ALWAYS_INLINE void EnableInterrupt() {
                cpu::CounterTimerPhysicalTimerControlRegisterAccessor(0).SetEnable(true).SetIMask(false).Store();
            }

            static ALWAYS_INLINE void DisableInterrupt() {
                cpu::CounterTimerPhysicalTimerControlRegisterAccessor(0).SetEnable(true).SetIMask(true).Store();
            }

            static ALWAYS_INLINE void StopTimer() {
                /* Set the compare value to the maximum. */
                cpu::CounterTimerPhysicalTimerCompareValueRegisterAccessor(0).SetCompareValue(std::numeric_limits<u64>::max()).Store();

                /* Disable the physical timer. */
                cpu::CounterTimerPhysicalTimerControlRegisterAccessor(0).SetEnable(false).SetIMask(false).Store();
            }

            static ALWAYS_INLINE s64 GetCount() {
                return cpu::CounterTimerPhysicalCountValueRegisterAccessor().GetCount();
            }

            static ALWAYS_INLINE void SetCompareValue(s64 value) {
                cpu::CounterTimerPhysicalTimerCompareValueRegisterAccessor(0).SetCompareValue(static_cast<u64>(value)).Store();
            }
        public:
            virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override {
                MESOSPHERE_UNUSED(interrupt_id);
                return this;
            }

            virtual void DoTask() override;
    };

}
