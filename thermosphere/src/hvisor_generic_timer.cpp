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

#include "hvisor_generic_timer.hpp"
#include "hvisor_irq_manager.hpp"
#include "hvisor_core_context.hpp"

#include "cpu/hvisor_cpu_interrupt_mask_guard.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

#include <mutex>

namespace ams::hvisor {

    void GenericTimer::Initialize()
    {
        Configure(false, false);
        if (currentCoreCtx->IsBootCore()) {
            m_timerFreq = THERMOSPHERE_GET_SYSREG(cntfrq_el0);
        }

        IrqManager::GetInstance().Register(*this, irqId, true);
    }

    std::optional<bool> GenericTimer::InterruptTopHalfHandler(u32 irqId, u32)
    {
        if (irqId != GenericTimer::irqId) {
            return std::nullopt;
        }

        // Mask the timer interrupt until reprogrammed
        Configure(false, false);

        return false;
    }

    void GenericTimer::WaitTicks(s64 ticks)
    {
        IrqManager::EnterInterruptibleHypervisorCode();
        auto flags = cpu::UnmaskIrq();
        SetTimeoutTicks(ticks);
        do {
            cpu::wfi();
        } while (!GetInterruptStatus());
        cpu::RestoreInterruptFlags(flags);
    }

}
