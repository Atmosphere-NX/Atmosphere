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

namespace ams::kern::arm64 {

    class KInterruptManager {
        NON_COPYABLE(KInterruptManager);
        NON_MOVEABLE(KInterruptManager);
        private:
            struct KCoreLocalInterruptEntry {
                KInterruptHandler *handler;
                bool manually_cleared;
                bool needs_clear;
                u8 priority;
            };

            struct KGlobalInterruptEntry {
                KInterruptHandler *handler;
                bool manually_cleared;
                bool needs_clear;
            };
        private:
            static inline KSpinLock s_lock;
            static inline KGlobalInterruptEntry s_global_interrupts[KInterruptController::NumGlobalInterrupts];
            static inline KInterruptController::GlobalState s_global_state;
            static inline bool s_global_state_saved;
        private:
            KCoreLocalInterruptEntry core_local_interrupts[KInterruptController::NumLocalInterrupts];
            KInterruptController interrupt_controller;
            KInterruptController::LocalState local_state;
            bool local_state_saved;
        public:
            KInterruptManager() : local_state_saved(false) { /* Leave things mostly uninitalized. We'll call ::Initialize() later. */ }
            /* TODO: Actually implement KInterruptManager functionality. */
        public:
            static ALWAYS_INLINE u32 DisableInterrupts() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif" : [intr_state]"=r"(intr_state));
                __asm__ __volatile__("msr daif, %[intr_state]" :: [intr_state]"r"(intr_state | 0x80));
                return intr_state;
            }

            static ALWAYS_INLINE u32 EnableInterrupts() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif" : [intr_state]"=r"(intr_state));
                __asm__ __volatile__("msr daif, %[intr_state]" :: [intr_state]"r"(intr_state & 0x7F));
                return intr_state;
            }

            static ALWAYS_INLINE void RestoreInterrupts(u32 intr_state) {
                u64 cur_state;
                __asm__ __volatile__("mrs %[cur_state], daif" : [cur_state]"=r"(cur_state));
                __asm__ __volatile__("msr daif, %[intr_state]" :: [intr_state]"r"((cur_state & 0x7F) | (intr_state & 0x80)));
            }

            static ALWAYS_INLINE bool AreInterruptsEnabled() {
                u64 intr_state;
                __asm__ __volatile__("mrs %[intr_state], daif" : [intr_state]"=r"(intr_state));
                return (intr_state & 0x80) == 0;
            }
    };

}
