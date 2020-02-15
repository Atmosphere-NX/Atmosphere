/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_gicv2.hpp"
#include "hvisor_synchronization.hpp"
#include "memory_map.h"

#include "exceptions.h" // TODO

namespace ams::hvisor {

    class IrqManager final {
        SINGLETON(IrqManager);
        friend class VirtualGic;
        private:
            static constexpr u8 hostPriority = 0;
            static constexpr u8 guestPriority = 1;

            static inline volatile auto *const gicd = (volatile GicV2Distributor *)MEMORY_MAP_VA_GICD; 
            static inline volatile auto *const gicc = (volatile GicV2Controller *)MEMORY_MAP_VA_GICC; 
            static inline volatile auto *const gich = (volatile GicV2VirtualInterfaceController *)MEMORY_MAP_VA_GICH;

            static bool IsGuestInterrupt(u32 id);

            static u32 GetTypeRegister()                                { return gicd->typer; }
            static void SetInterruptEnabled(u32 id)                     { gicd->isenabler[id / 32] = BIT(id % 32); }
            static void ClearInterruptEnabled(u32 id)                   { gicd->icenabler[id / 32] = BIT(id % 32); }
            static bool IsInterruptPending(u32 id)                      { return (gicd->ispendr[id / 32] & BIT(id % 32)) != 0;}
            static void ClearInterruptPending(u32 id)                   { gicd->icpendr[id / 32] = BIT(id % 32); }
            static void ClearInterruptActive(u32 id)                    { gicd->icactiver[id / 32] = BIT(id % 32); }
            static void SetInterruptShiftedPriority(u32 id, u8 prio)    { gicd->ipriorityr[id] = prio; }
            static void SetInterruptTargets(u32 id, u8 targetList)      { gicd->itargetsr[id] = targetList; }
            static bool IsInterruptEdgeTriggered(u32 id)
            {
                return ((gicd->icfgr[id / 16] >> GicV2Distributor::GetCfgrShift(id)) & 2) != 0;
            }
            static void SetInterruptMode(u32 id, bool isEdgeTriggered)
            {
                u32 cfgw = gicd->icfgr[id / 16];
                cfgw &= ~(2 << GicV2Distributor::GetCfgrShift(id));
                cfgw |= (isEdgeTriggered ? 2 : 0) << GicV2Distributor::GetCfgrShift(id);
                gicd->icfgr[id / 16] = cfgw;
            }

            static u32 AcknowledgeIrq()                         { return gicc->iar; }
            static void DropCurrentInterruptPriority(u32 iar)   { gicc->eoir = iar; }
            static void DeactivateCurrentInterrupt(u32 iar)     { gicc->dir = iar; }

        private:
            mutable RecursiveSpinlock m_lock{};
            u32 m_numSharedInterrupts = 0;
            u8 m_priorityShift = 0;
            u8 m_numPriorityLevels = 0;
            u8 m_numCpuInterfaces = 0;

        private:
            void SetInterruptPriority(u32 id, u8 prio) { SetInterruptShiftedPriority(id, prio << m_priorityShift); }

            void InitializeGic();
            void DoConfigureInterrupt(u32 id, u8 prio, bool isLevelSensitive);

        public:
            enum ThermosphereSgi : u32 {
                ExecuteFunctionSgi = 0,
                VgicUpdateSgi,
                DebugPauseSgi,
                ReportDebuggerBreakSgi,
                DebuggerContinueSgi,

                MaxSgi,
            };

            static void GenerateSgiForList(ThermosphereSgi id, u32 coreList)
            {
                gicd->sgir = GicV2Distributor::ForwardToTargetList << 24 | coreList << 16 | id;
            }
            static void GenerateSgiForAllOthers(ThermosphereSgi id)
            {
                gicd->sgir = GicV2Distributor::ForwardToAllOthers << 24 | id;
            }

            static void HandleInterrupt(ExceptionStackFrame *frame);

        public:
            void Initialize();
            void ConfigureInterrupt(u32 id, u8 prio, bool isLevelSensitive);
            void SetInterruptAffinity(u32 id, u8 affinityMask);

        public:
            constexpr IrqManager() = default;
    };

}
