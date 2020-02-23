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


#include "hvisor_hw_stop_point_manager.hpp"
#include "hvisor_core_context.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"
#include "cpu/hvisor_cpu_interrupt_mask_guard.hpp"
#include <mutex>

#define _REENT_ONLY
#include <cerrno>

namespace ams::hvisor {

    void HwStopPointManager::DoReloadOnAllCores() const
    {
        cpu::InterruptMaskGuard mg{};
        cpu::dmb();
        Reload();
        m_reloadBarrier.Reset(CoreContext::GetActiveCoreMask());
        IrqManager::GenerateSgiForAllOthers(m_irqId);
        m_reloadBarrier.Join();
    }

    cpu::DebugRegisterPair *HwStopPointManager::Allocate()
    {
        size_t pos = __builtin_ffs(m_freeBitmap);
        if (pos == 0) {
            return nullptr;
        } else {
            m_freeBitmap &= ~BIT(pos - 1);
            m_usedBitmap |= BIT(pos - 1);
            return &m_stopPoints[pos - 1];
        }
    }

    void HwStopPointManager::Free(size_t pos)
    {
        m_stopPoints[pos] = {};
        m_freeBitmap |= BIT(pos);
        m_usedBitmap &= ~BIT(pos);
    }

    const cpu::DebugRegisterPair *HwStopPointManager::Find(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl dir) const
    {
        for (auto bit: util::BitsOf{m_usedBitmap}) {
            auto *p = &m_stopPoints[bit];
            if (FindPredicate(*p, addr, size, dir)) {
                return p;
            }
        }

        return nullptr;
    }

    int HwStopPointManager::AddImpl(uintptr_t addr, size_t size, cpu::DebugRegisterPair preconfiguredPair)
    {
        std::scoped_lock lk{m_lock};
        auto lsc = preconfiguredPair.cr.lsc;

        if (m_freeBitmap == 0) {
            // Oops
            return -EBUSY;
        }

        if (Find(addr, size, lsc) != nullptr) {
            // Already exists
            return -EEXIST;
        }

        auto *regs = Allocate();
        regs->SetDefaults();

        // Apply preconfig
        regs->cr.raw |= preconfiguredPair.cr.raw;
        regs->vr = preconfiguredPair.vr;
        regs->cr.enabled = true;

        DoReloadOnAllCores();
        return 0;
    }

    int HwStopPointManager::RemoveImpl(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl dir)
    {
        std::scoped_lock lk{m_lock};
        const auto *p = Find(addr, size, dir);
        if (p == nullptr) {
            return -ENOENT;
        }

        Free(p - m_stopPoints.data());
        return 0;
    }

    void HwStopPointManager::RemoveAll()
    {
        std::scoped_lock lk{m_lock};
        m_freeBitmap |= m_usedBitmap;
        m_usedBitmap = 0;
        std::fill(m_stopPoints.begin(), m_stopPoints.end(), cpu::DebugRegisterPair{});
        DoReloadOnAllCores();
    }

    std::optional<bool> HwStopPointManager::InterruptTopHalfHandler(u32 irqId, u32)
    {
        if (irqId != m_irqId) {
            return {};
        }

        Reload();
        return false;
    }
}
