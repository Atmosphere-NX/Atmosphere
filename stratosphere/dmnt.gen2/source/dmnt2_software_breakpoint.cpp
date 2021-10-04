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
#include "dmnt2_software_breakpoint.hpp"
#include "dmnt2_debug_process.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    namespace {

        constexpr const u8 Aarch64BreakInstruction[]      = {0xFF, 0xFF, 0xFF, 0xE7};
        constexpr const u8 Aarch32BreakInstruction[]      = {0xFE, 0xDE, 0xFF, 0xE7};
        constexpr const u8 Aarch32ThumbBreakInstruction[] = {0x80, 0xB6};

    }

    Result SoftwareBreakPoint::Clear(DebugProcess *debug_process) {
        Result result = svc::ResultInvalidArgument();
        if (m_in_use) {
            if (m_address) {
                result = debug_process->WriteMemory(std::addressof(m_insn), m_address, m_size);
                if (R_SUCCEEDED(result)) {
                    AMS_DMNT2_GDB_LOG_DEBUG("SoftwareBreakPoint::Clear %p 0x%lx, insn=0x%x\n", this, m_address, m_insn);
                } else {
                    AMS_DMNT2_GDB_LOG_DEBUG("SoftwareBreakPoint::Clear %p 0x%lx, insn=0x%x, !!! Fail %08x !!!\n", this, m_address, m_insn, result.GetValue());
                }
                this->Reset();
            } else {
                AMS_DMNT2_GDB_LOG_ERROR("SoftwareBreakPoint::Clear %p 0x%lx, insn=0x%x, !!! Null Address !!!\n", this, m_address, m_insn);
            }
        }
        return result;
    }

    Result SoftwareBreakPoint::Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool is_step) {
        /* Set fields. */
        m_is_step = is_step;
        m_address = address;
        m_size    = size;

        /* Read our instruction. */
        Result result = debug_process->ReadMemory(std::addressof(m_insn), m_address, m_size);
        AMS_DMNT2_GDB_LOG_DEBUG("SoftwareBreakPoint::Set %p 0x%lx, insn=0x%x\n", this, m_address, m_insn);

        /* Set the breakpoint. */
        if (debug_process->Is64Bit()) {
            if (m_size == sizeof(Aarch64BreakInstruction)) {
                result = debug_process->WriteMemory(Aarch64BreakInstruction, m_address, m_size);
            } else {
                result = svc::ResultInvalidArgument();
            }
        } else {
            if (m_size == sizeof(Aarch32BreakInstruction) || m_size == sizeof(Aarch32ThumbBreakInstruction)) {
                result = debug_process->WriteMemory(m_size == sizeof(Aarch32BreakInstruction) ? Aarch32BreakInstruction : Aarch32ThumbBreakInstruction, m_address, m_size);
            } else {
                result = svc::ResultInvalidArgument();
            }
        }

        /* Check that we succeeded. */
        if (R_SUCCEEDED(result)) {
            m_in_use = true;
        }

        return result;
    }

    SoftwareBreakPointManager::SoftwareBreakPointManager(DebugProcess *debug_process) : BreakPointManager(debug_process) {
        /* ... */
    }

    BreakPointBase *SoftwareBreakPointManager::GetBreakPoint(size_t index) {
        if (index < util::size(m_breakpoints)) {
            return m_breakpoints + index;
        } else {
            return nullptr;
        }
    }


}
