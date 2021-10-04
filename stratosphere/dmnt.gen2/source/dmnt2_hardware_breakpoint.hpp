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
#include "dmnt2_breakpoint_manager.hpp"

namespace ams::dmnt {

    struct HardwareBreakPoint : public BreakPoint {
        svc::HardwareBreakPointRegisterName m_reg;
        svc::HardwareBreakPointRegisterName m_ctx;

        void Initialize(svc::HardwareBreakPointRegisterName r, svc::HardwareBreakPointRegisterName c) {
            m_reg = r;
            m_ctx = c;
        }

        virtual Result Clear(DebugProcess *debug_process) override;
        virtual Result Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool is_step) override;
    };

    class HardwareBreakPointManager : public BreakPointManager {
        public:
            static constexpr size_t BreakPointCountMax = 0x10;
        private:
            HardwareBreakPoint m_breakpoints[BreakPointCountMax];
        public:
            static Result SetHardwareBreakPoint(u32 r, u64 dbgbcr, u64 value);
            static Result SetContextBreakPoint(svc::HardwareBreakPointRegisterName ctx, DebugProcess *debug_process);
            static svc::HardwareBreakPointRegisterName GetWatchPointContextRegister();
            static Result SetExecutionBreakPoint(svc::HardwareBreakPointRegisterName reg, svc::HardwareBreakPointRegisterName ctx, u64 address);
        public:
            explicit HardwareBreakPointManager(DebugProcess *debug_process);
        private:
            virtual BreakPointBase *GetBreakPoint(size_t index) override;
        private:
            static void CountBreakPointRegisters();

            static bool SendMultiCoreRequest(const void *request);
    };

}