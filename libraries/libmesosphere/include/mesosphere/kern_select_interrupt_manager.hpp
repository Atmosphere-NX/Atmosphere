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
#include <mesosphere/kern_select_interrupt_name.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <mesosphere/arch/arm64/kern_k_interrupt_manager.hpp>
    namespace ams::kern {
        using ams::kern::arch::arm64::KInterruptManager;
    }

#else

    #error "Unknown architecture for KInterruptManager"

#endif


namespace ams::kern {


    /* Enable or disable interrupts for the lifetime of an object. */
    class KScopedInterruptDisable {
        NON_COPYABLE(KScopedInterruptDisable);
        NON_MOVEABLE(KScopedInterruptDisable);
        private:
            u32 prev_intr_state;
        public:
            ALWAYS_INLINE KScopedInterruptDisable() : prev_intr_state(KInterruptManager::DisableInterrupts()) { /* ... */ }
            ALWAYS_INLINE ~KScopedInterruptDisable() { KInterruptManager::RestoreInterrupts(prev_intr_state); }
    };

    class KScopedInterruptEnable {
        NON_COPYABLE(KScopedInterruptEnable);
        NON_MOVEABLE(KScopedInterruptEnable);
        private:
            u32 prev_intr_state;
        public:
            ALWAYS_INLINE KScopedInterruptEnable() : prev_intr_state(KInterruptManager::EnableInterrupts()) { /* ... */ }
            ALWAYS_INLINE ~KScopedInterruptEnable() { KInterruptManager::RestoreInterrupts(prev_intr_state); }
    };

}
