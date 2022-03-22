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

    class DebugProcess;

    struct BreakPoint : public BreakPointBase {
        bool m_is_step;

        BreakPoint() { /* ... */ }

        virtual Result Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool is_step) = 0;
    };

    class BreakPointManager : public BreakPointManagerBase {
        public:
            explicit BreakPointManager(DebugProcess *debug_process);

            void ClearStep();

            Result SetBreakPoint(uintptr_t address, size_t size, bool is_step);
    };

}