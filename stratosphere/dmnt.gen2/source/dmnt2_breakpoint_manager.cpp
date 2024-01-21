/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "dmnt2_breakpoint_manager.hpp"
#include "dmnt2_debug_process.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    BreakPointManager::BreakPointManager(DebugProcess *debug_process) : BreakPointManagerBase(debug_process) {
        /* ... */
    }

    void BreakPointManager::ClearStep() {
        BreakPoint *bp = nullptr;
        for (size_t i = 0; (bp = static_cast<BreakPoint *>(this->GetBreakPoint(i))) != nullptr; ++i) {
            if (bp->m_in_use && bp->m_is_step) {
                AMS_DMNT2_GDB_LOG_DEBUG("BreakPointManager::ClearStep %p 0x%lx (idx=%zu)\n", bp, bp->m_address, i);
                bp->Clear(m_debug_process);
            }
        }
    }

    Result BreakPointManager::SetBreakPoint(uintptr_t address, size_t size, bool is_step) {
        /* Get a free breakpoint. */
        BreakPoint *bp = static_cast<BreakPoint *>(this->GetFreeBreakPoint());

        /* Set the breakpoint. */
        Result result = svc::ResultOutOfHandles();
        if (bp != nullptr) {
            result = bp->Set(m_debug_process, address, size, is_step);
        }

        if (R_FAILED(result)) {
            AMS_DMNT2_GDB_LOG_DEBUG("BreakPointManager::SetBreakPoint %p 0x%lx !!! Fail 0x%08x !!!\n", bp, address, result.GetValue());
        }

        R_RETURN(result);
    }

}
