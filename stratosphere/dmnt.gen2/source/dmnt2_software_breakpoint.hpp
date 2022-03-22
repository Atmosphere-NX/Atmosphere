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

    struct SoftwareBreakPoint : public BreakPoint {
        u32 m_insn;

        virtual Result Clear(DebugProcess *debug_process) override;
        virtual Result Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool is_step) override;
    };

    class SoftwareBreakPointManager : public BreakPointManager {
        public:
            static constexpr size_t BreakPointCountMax = 0x80;
        private:
            SoftwareBreakPoint m_breakpoints[BreakPointCountMax];
        public:
            explicit SoftwareBreakPointManager(DebugProcess *debug_process);
        private:
            virtual BreakPointBase *GetBreakPoint(size_t index) override;
    };

}