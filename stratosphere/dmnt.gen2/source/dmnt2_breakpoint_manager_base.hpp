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

namespace ams::dmnt {

    class DebugProcess;

    struct BreakPointBase {
        bool m_in_use;
        uintptr_t m_address;
        size_t m_size;

        BreakPointBase() : m_in_use(false) { /* ... */ }

        void Reset() { m_in_use = false; }

        virtual Result Clear(DebugProcess *debug_process) = 0;
    };

    class BreakPointManagerBase {
        protected:
            DebugProcess *m_debug_process;
        public:
            explicit BreakPointManagerBase(DebugProcess *debug_process);

            void ClearAll();
            void Reset();

            Result ClearBreakPoint(uintptr_t address, size_t size);
        protected:
            virtual BreakPointBase *GetBreakPoint(size_t index) = 0;
            BreakPointBase *GetFreeBreakPoint();
    };

}