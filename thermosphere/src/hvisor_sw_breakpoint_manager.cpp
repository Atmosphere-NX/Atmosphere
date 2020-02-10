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

#include "hvisor_sw_breakpoint_manager.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

#include <mutex>

#include "guest_memory.h"

#define _REENT_ONLY
#include <cerrno>

/*
    Consider the following:
        - Breakpoints are based on VA
        - Translation tables may change
        - Translation tables may differ from core to core

    We also define sw breakpoints on invalid addresses (for one or more cores) UNPREDICTABLE.
*/

namespace ams::hvisor {

    SwBreakpointManager SwBreakpointManager::instance{};

    size_t SwBreakpointManager::FindClosest(uintptr_t addr) const
    {
        auto endit = m_breakpoints.cbegin() + m_numBreakpoints;
        auto it = std::lower_bound(
            m_breakpoints.cbegin(),
            endit,
            addr,
            [] (const Breakpoint &a, const Breakpoint &b) {
                return a.address < b.address;
            }
        );

        return it == endit ? m_numBreakpoints : static_cast<size_t>(it - m_breakpoints.cbegin());
    }

    bool SwBreakpointManager::DoApply(size_t id)
    {
        Breakpoint &bp = m_breakpoints[id];
        u32 brkInst = 0xD4200000 | (bp.uid << 5);

        size_t sz = guestReadWriteMemory(bp.address, 4, &bp.savedInstruction, &brkInst);
        bp.applied = sz == 4;
        m_triedToApplyOrRevertBreakpoint.store(true);
        return sz == 4;
    }

    bool SwBreakpointManager::DoRevert(size_t id)
    {
        Breakpoint &bp = m_breakpoints[id];
        size_t sz = guestWriteMemory(bp.address, 4, &bp.savedInstruction);
        bp.applied = sz != 4;
        m_triedToApplyOrRevertBreakpoint.store(true);
        return sz == 4;
    }

    // TODO apply revert handlers

    int SwBreakpointManager::Add(uintptr_t addr, bool persistent)
    {
        if ((addr & 3) != 0) {
            return -EINVAL;
        }

        std::scoped_lock lk{m_lock};

        if (m_numBreakpoints == MAX_SW_BREAKPOINTS) {
            return -EBUSY;
        }

        size_t id = FindClosest(addr);
        if (id != m_numBreakpoints && m_breakpoints[id].uid != 0) {
            return -EEXIST;
        }

        // Insert
        for(size_t i = m_numBreakpoints; i > id && i != 0; i--) {
            m_breakpoints[i] = m_breakpoints[i - 1];
        }
        ++m_numBreakpoints;

        Breakpoint &bp = m_breakpoints[id];
        bp.address = addr;
        bp.persistent = persistent;
        bp.applied = false;
        bp.uid = static_cast<u16>(0x2000 + m_bpUniqueCounter++);

        return Apply(id) ? 0 : -EFAULT;
    }

    int SwBreakpointManager::Remove(uintptr_t addr, bool keepPersistent)
    {
        if ((addr & 3) != 0) {
            return -EINVAL;
        }

        std::scoped_lock lk{m_lock};

        if (m_numBreakpoints == MAX_SW_BREAKPOINTS) {
            return -EBUSY;
        }

        size_t id = FindClosest(addr);
        if (id == m_numBreakpoints || m_breakpoints[id].uid == 0) {
            return -ENOENT;
        }

        Breakpoint &bp = m_breakpoints[id];
        bool ok = true;
        if (!keepPersistent || !bp.persistent) {
            ok = Revert(id);
        }

        for(size_t i = id; i < m_numBreakpoints - 1; i++) {
            m_breakpoints[i] = m_breakpoints[i + 1];
        }

        m_breakpoints[--m_numBreakpoints] = {};

        return ok ? 0 : -EFAULT;
    }

    int SwBreakpointManager::RemoveAll(bool keepPersistent)
    {
        std::scoped_lock lk{m_lock};

        bool ok = true;
        for (size_t id = 0; id < m_numBreakpoints; id++) {
            Breakpoint &bp = m_breakpoints[id];
            if (!keepPersistent || !bp.persistent) {
                ok = ok && Revert(id);
            }
        }

        std::fill_n(m_breakpoints.begin(), m_breakpoints.end(), Breakpoint{});
        m_numBreakpoints = 0;
        m_bpUniqueCounter = 0;

        return ok ? 0 : -EFAULT;
    }


}