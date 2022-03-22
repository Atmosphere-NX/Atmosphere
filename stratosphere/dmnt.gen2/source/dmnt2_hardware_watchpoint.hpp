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
#pragma once
#include <stratosphere.hpp>
#include "dmnt2_breakpoint_manager_base.hpp"

namespace ams::dmnt {

    struct WatchPoint : public BreakPointBase {
        svc::HardwareBreakPointRegisterName m_reg;
        svc::HardwareBreakPointRegisterName m_ctx;
        bool m_read;
        bool m_write;

        void Initialize(svc::HardwareBreakPointRegisterName r, svc::HardwareBreakPointRegisterName c) {
            m_reg = r;
            m_ctx = c;
        }

        virtual Result Clear(DebugProcess *debug_process) override;
        Result Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool read, bool write);
    };

    class HardwareWatchPointManager : public BreakPointManagerBase {
        public:
            static constexpr size_t BreakPointCountMax = 0x10;
        private:
            WatchPoint m_breakpoints[BreakPointCountMax];
        public:
            static bool IsValidWatchPoint(u64 address, u64 size);
        public:
            explicit HardwareWatchPointManager(DebugProcess *debug_process);

            Result SetWatchPoint(u64 address, u64 size, bool read, bool write);
            Result GetWatchPointInfo(u64 address, bool &read, bool &write);
        private:
            virtual BreakPointBase *GetBreakPoint(size_t index) override;
    };

}