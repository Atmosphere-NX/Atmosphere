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

#include "hvisor_hw_breakpoint_manager.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

#define _REENT_ONLY
#include <cerrno>

// Can't use two THERMOSPHERE_SAVE_SYSREG as it prevents ldp from being generated
#define SAVE_BREAKPOINT(i, _)\
    __asm__ __volatile__ (\
        "msr " STRINGIZE(dbgbvr##i##_el1) ", %0\n"\
        "msr " STRINGIZE(dbgbcr##i##_el1) ", %1"\
        :\
        : "r"(m_stopPoints[i].vr), "r"(m_stopPoints[i].cr.raw)\
        : "memory"\
    );

namespace ams::hvisor {

    HwBreakpointManager HwBreakpointManager::instance{};

    void HwBreakpointManager::Reload() const
    {
        cpu::dmb();
        EVAL(REPEAT(MAX_BCR, SAVE_BREAKPOINT, ~));
        cpu::dsb();
        cpu::isb();
    }

    bool HwBreakpointManager::FindPredicate(const cpu::DebugRegisterPair &pair, uintptr_t addr, size_t, cpu::DebugRegisterPair::LoadStoreControl) const
    {
        return pair.vr == addr;
    }

    // Note: A32/T32/T16 support intentionnally left out
    // Note: addresses are supposed to be well-formed regarding the sign extension bits
    int HwBreakpointManager::Add(uintptr_t addr)
    {
        // Reject misaligned addresses
        if (addr & 3) {
            return -EINVAL;
        }

        cpu::DebugRegisterPair bp{};
        bp.cr.bt = cpu::DebugRegisterPair::AddressMatch;
        bp.cr.bas = 0xF; // mandated
        bp.vr = addr;

        return AddImpl(addr, 0, bp);
    }

    int HwBreakpointManager::Remove(uintptr_t addr)
    {
        // Reject misaligned addresses
        if (addr & 3) {
            return -EINVAL;
        }

        return RemoveImpl(addr, 0, cpu::DebugRegisterPair::NotAWatchpoint);
    }
}
