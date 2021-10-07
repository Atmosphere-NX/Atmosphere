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
#include "dmnt2_hardware_watchpoint.hpp"
#include "dmnt2_hardware_breakpoint.hpp"
#include "dmnt2_debug_process.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    namespace {

        Result SetDataBreakPoint(svc::HardwareBreakPointRegisterName reg, svc::HardwareBreakPointRegisterName ctx, u64 address, u64 size, bool read, bool write) {
            /* Determine lsc. */
            const u8 lsc = (read ? 1 : 0) | (write ? 2 : 0);

            /* Check that the watchpoint is valid. */
            if (lsc != 0) {
                R_UNLESS(HardwareWatchPointManager::IsValidWatchPoint(address, size), svc::ResultInvalidArgument());
            }

            /* Determine bas/mask. */
            u8 bas = 0, mask = 0;
            if (size <= 8) {
                bas     = ((1 << size) - 1) << (address & 7);
                address = util::AlignDown(address, 8);
            } else {
                bas  = 0xFF;
                mask = util::PopCount(size - 1);
            }

            /* Build dbgbcr value. */
            const u64 dbgbcr = (mask << 24) | (ctx << 16) | (bas << 5) | (lsc << 3) | ((lsc != 0) ? 1 : 0);

            /* Set the breakpoint. */
            const Result result = HardwareBreakPointManager::SetHardwareBreakPoint(reg, dbgbcr, address);
            if (R_FAILED(result)) {
                AMS_DMNT2_GDB_LOG_ERROR("SetDataBreakPoint FAIL 0x%08x, reg=%d, address=0x%lx\n", result.GetValue(), reg, address);
            }
            return result;
        }

    }

    bool HardwareWatchPointManager::IsValidWatchPoint(u64 address, u64 size) {
        /* Check size. */
        if (size == 0) {
            AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::IsValidWatchPoint(%lx, %lx) FAIL size == 0\n", address, size);
            return false;
        }

        /* Validate. */
        if (size <= 8) {
            /* Check that address is aligned. */
            if (util::AlignDown(address, 8) != util::AlignDown(address + size - 1, 8)) {
                AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::IsValidWatchPoint(%lx, %lx) FAIL range crosses qword boundary\n", address, size);
                return false;
            }
        } else {
            /* Check size is small enough. */
            if (size > 0x80000000) {
                AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::IsValidWatchPoint(%lx, %lx) FAIL size too big\n", address, size);
                return false;
            }

            /* Check size is power of two. */
            if (!util::IsPowerOfTwo(size)) {
                AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::IsValidWatchPoint(%lx, %lx) FAIL size not power of two\n", address, size);
                return false;
            }

            /* Check alignment. */
            if (!util::IsAligned(address, size)) {
                AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::IsValidWatchPoint(%lx, %lx) FAIL address not size-aligned\n", address, size);
                return false;
            }
        }

        return true;
    }

    Result WatchPoint::Clear(DebugProcess *debug_process) {
        AMS_UNUSED(debug_process);

        Result result = svc::ResultInvalidArgument();
        if (m_in_use) {
            AMS_DMNT2_GDB_LOG_DEBUG("WatchPoint::Clear %p 0x%lx\n", this, m_address);
            result = SetDataBreakPoint(m_reg, m_ctx, 0, 0, false, false);
            this->Reset();
        }
        return result;
    }

    Result WatchPoint::Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool read, bool write) {
        /* Set fields. */
        m_address = address;
        m_size    = size;
        m_read    = read;
        m_write   = write;

        /* Set context breakpoint. */
        R_TRY(HardwareBreakPointManager::SetContextBreakPoint(m_ctx, debug_process));

        /* Set watchpoint. */
        R_TRY(SetDataBreakPoint(m_reg, m_ctx, address, size, read, write));

        /* Set as in-use. */
        m_in_use = true;
        return ResultSuccess();
    }

    HardwareWatchPointManager::HardwareWatchPointManager(DebugProcess *debug_process) : BreakPointManagerBase(debug_process) {
        const svc::HardwareBreakPointRegisterName ctx = HardwareBreakPointManager::GetWatchPointContextRegister();
        for (size_t i = 0; i < util::size(m_breakpoints); ++i) {
            m_breakpoints[i].Initialize(static_cast<svc::HardwareBreakPointRegisterName>(svc::HardwareBreakPointRegisterName_D0 + i), ctx);
        }
    }

    BreakPointBase *HardwareWatchPointManager::GetBreakPoint(size_t index) {
        if (index < util::size(m_breakpoints)) {
            return m_breakpoints + index;
        } else {
            return nullptr;
        }
    }

    Result HardwareWatchPointManager::SetWatchPoint(u64 address, u64 size, bool read, bool write) {
        /* Get a free watchpoint. */
        auto *bp = static_cast<WatchPoint *>(this->GetFreeBreakPoint());
        R_UNLESS(bp != nullptr, svc::ResultOutOfHandles());

        /* Set the watchpoint. */
        return bp->Set(m_debug_process, address, size, read, write);
    }

    Result HardwareWatchPointManager::GetWatchPointInfo(u64 address, bool &read, bool &write) {
        /* Find a matching watchpoint. */
        for (const auto &bp : m_breakpoints) {
            if (bp.m_in_use) {
                if (bp.m_address <= address && address < bp.m_address + bp.m_size) {
                    read  = bp.m_read;
                    write = bp.m_write;
                    return ResultSuccess();
                }
            }
        }

        /* Otherwise, we failed. */
        AMS_DMNT2_GDB_LOG_ERROR("HardwareWatchPointManager::GetWatchPointInfo FAIL 0x%lx\n", address);
        read  = false;
        write = false;

        return svc::ResultInvalidArgument();
    }

}
