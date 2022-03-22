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
#include "dmnt2_breakpoint_manager_base.hpp"
#include "dmnt2_debug_process.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    BreakPointManagerBase::BreakPointManagerBase(DebugProcess *debug_process) : m_debug_process(debug_process) {
        /* ... */
    }

    void BreakPointManagerBase::ClearAll() {
        BreakPointBase *bp = nullptr;
        for (size_t i = 0; (bp = static_cast<BreakPointBase *>(this->GetBreakPoint(i))) != nullptr; ++i) {
            if (bp->m_in_use) {
                bp->Clear(m_debug_process);
            }
        }
    }

    void BreakPointManagerBase::Reset() {
        BreakPointBase *bp = nullptr;
        for (size_t i = 0; (bp = static_cast<BreakPointBase *>(this->GetBreakPoint(i))) != nullptr; ++i) {
            bp->Reset();
        }
    }

    Result BreakPointManagerBase::ClearBreakPoint(uintptr_t address, size_t size) {
        BreakPointBase *bp = nullptr;
        for (size_t i = 0; (bp = static_cast<BreakPointBase *>(this->GetBreakPoint(i))) != nullptr; ++i) {
            if (bp->m_in_use && bp->m_address == address) {
                AMS_ABORT_UNLESS(bp->m_size == size);
                return bp->Clear(m_debug_process);
            }
        }
        return ResultSuccess();
    }

    BreakPointBase *BreakPointManagerBase::GetFreeBreakPoint() {
        BreakPointBase *bp = nullptr;
        for (size_t i = 0; (bp = static_cast<BreakPointBase *>(this->GetBreakPoint(i))) != nullptr; ++i) {
            if (!bp->m_in_use) {
                return bp;
            }
        }
        return nullptr;
    }

}
