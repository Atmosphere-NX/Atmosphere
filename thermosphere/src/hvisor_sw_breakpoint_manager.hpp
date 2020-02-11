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

#pragma once

#include "defines.hpp"
#include "hvisor_synchronization.hpp"

#define MAX_SW_BREAKPOINTS  16

namespace ams::hvisor {

    class SwBreakpointManager {
        SINGLETON(SwBreakpointManager);
        private:
            struct Breakpoint {
                uintptr_t address;
                u32 savedInstruction;
                u16 uid;
                bool persistent;
                bool applied;
            };

        private:
            mutable RecursiveSpinlock m_lock{};
            std::atomic<bool> m_triedToApplyOrRevertBreakpoint{};

            u32 m_bpUniqueCounter = 0;
            size_t m_numBreakpoints = 0;
            std::array<Breakpoint, MAX_SW_BREAKPOINTS> m_breakpoints{};

        private:
            size_t FindClosest(uintptr_t addr) const;

            bool DoApply(size_t id);
            bool DoRevert(size_t id);

            // TODO apply, revert handler
            bool Apply(size_t id);
            bool Revert(size_t id);

        public:
            int Add(uintptr_t addr, bool persistent);
            int Remove(uintptr_t addr, bool keepPersistent);
            int RemoveAll(bool keepPersistent);

        public: 
            constexpr SwBreakpointManager() = default;
    };
}
