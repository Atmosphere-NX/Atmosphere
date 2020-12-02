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
#include <vapours.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_interrupt_task.hpp>
#include <mesosphere/kern_select_interrupt_controller.hpp>

namespace ams::kern::arch::arm64 {

    class KInterruptManager {
        NON_COPYABLE(KInterruptManager);
        NON_MOVEABLE(KInterruptManager);
        private:
            struct KCoreLocalInterruptEntry {
                KInterruptHandler *handler;
                bool manually_cleared;
                bool needs_clear;
                u8 priority;

                constexpr KCoreLocalInterruptEntry()
                    : handler(nullptr), manually_cleared(false), needs_clear(false), priority(KInterruptController::PriorityLevel_Low)
                {
                    /* ... */
                }
            };

            struct KGlobalInterruptEntry {
                KInterruptHandler *handler;
                bool manually_cleared;
                bool needs_clear;

                constexpr KGlobalInterruptEntry() : handler(nullptr), manually_cleared(false), needs_clear(false) { /* ... */ }
            };
        private:
            KCoreLocalInterruptEntry core_local_interrupts[cpu::NumCores][KInterruptController::NumLocalInterrupts]{};
            KInterruptController interrupt_controller{};
            KInterruptController::LocalState local_states[cpu::NumCores]{};
            bool local_state_saved[cpu::NumCores]{};
            mutable KSpinLock global_interrupt_lock{};
            KGlobalInterruptEntry global_interrupts[KInterruptController::NumGlobalInterrupts]{};
            KInterruptController::GlobalState global_state{};
            bool global_state_saved{};
        private:
            ALWAYS_INLINE KSpinLock &GetGlobalInterruptLock() const { return this->global_interrupt_lock; }
            ALWAYS_INLINE KGlobalInterruptEntry &GetGlobalInterruptEntry(s32 irq) { return this->global_interrupts[KInterruptController::GetGlobalInterruptIndex(irq)]; }
            ALWAYS_INLINE KCoreLocalInterruptEntry &GetLocalInterruptEntry(s32 irq) { return this->core_local_interrupts[GetCurrentCoreId()][KInterruptController::GetLocalInterruptIndex(irq)]; }

            bool OnHandleInterrupt();
        public:
            constexpr KInterruptManager() = default;

            NOINLINE void Initialize(s32 core_id);
            NOINLINE void Finalize(s32 core_id);

            NOINLINE void Save(s32 core_id);
            NOINLINE void Restore(s32 core_id);

            bool IsInterruptDefined(s32 irq) const {
                return this->interrupt_controller.IsInterruptDefined(irq);
            }

            bool IsGlobal(s32 irq) const {
                return this->interrupt_controller.IsGlobal(irq);
            }

            bool IsLocal(s32 irq) const {
                return this->interrupt_controller.IsLocal(irq);
            }

            NOINLINE Result BindHandler(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level);
            NOINLINE Result UnbindHandler(s32 irq, s32 core);

            NOINLINE Result ClearInterrupt(s32 irq);
            NOINLINE Result ClearInterrupt(s32 irq, s32 core_id);

            ALWAYS_INLINE void SendInterProcessorInterrupt(s32 irq, u64 core_mask) {
                this->interrupt_controller.SendInterProcessorInterrupt(irq, core_mask);
            }

            ALWAYS_INLINE void SendInterProcessorInterrupt(s32 irq) {
                this->interrupt_controller.SendInterProcessorInterrupt(irq);
            }

            static void HandleInterrupt(bool user_mode);

            /* Implement more KInterruptManager functionality. */
        private:
            Result BindGlobal(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level);
            Result BindLocal(KInterruptHandler *handler, s32 irq, s32 priority, bool manual_clear);
            Result UnbindGlobal(s32 irq);
            Result UnbindLocal(s32 irq);
            Result ClearGlobal(s32 irq);
            Result ClearLocal(s32 irq);
        public:
            static ALWAYS_INLINE u32 DisableInterrupts() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif\n"
                                     "msr daifset, #2"
                                     : [intr_state]"=r"(intr_state)
                                     :: "memory");
                return intr_state;
            }

            static ALWAYS_INLINE u32 EnableInterrupts() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif\n"
                                     "msr daifclr, #2"
                                     : [intr_state]"=r"(intr_state)
                                     :: "memory");
                return intr_state;
            }

            static ALWAYS_INLINE void RestoreInterrupts(u32 intr_state) {
                u64 cur_state;
                __asm__ __volatile__("mrs %[cur_state], daif" : [cur_state]"=r"(cur_state));
                __asm__ __volatile__("msr daif, %[intr_state]" :: [intr_state]"r"((cur_state & ~0x80ul) | (intr_state & 0x80)));
            }

            static ALWAYS_INLINE bool AreInterruptsEnabled() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif" : [intr_state]"=r"(intr_state));
                return (intr_state & 0x80) == 0;
            }
    };

}
