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
#include "cpu/hvisor_cpu_debug_register_pair.hpp"

namespace ams::hvisor {

    class HwStopPointManager {
        NON_COPYABLE(HwStopPointManager);
        NON_MOVEABLE(HwStopPointManager);
        protected:
            static constexpr size_t maxStopPoints = std::max(MAX_BCR, MAX_WCR);

            mutable RecursiveSpinlock m_lock{};
            u16 m_freeBitmap;
            u16 m_usedBitmap = 0;
            std::array<cpu::DebugRegisterPair, maxStopPoints> m_stopPoints{};

        protected:
            cpu::DebugRegisterPair *Allocate();
            void Free(size_t pos);
            const cpu::DebugRegisterPair *Find(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl dir) const;

            virtual bool FindPredicate(const cpu::DebugRegisterPair &pair, uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl direction) const = 0;

            int AddImpl(uintptr_t addr, size_t size, cpu::DebugRegisterPair preconfiguredPair);
            int RemoveImpl(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl direction);

        protected:
            constexpr HwStopPointManager(size_t numStopPoints) : m_freeBitmap(MASK(numStopPoints)) {}

        public:
            virtual void ReloadOnAllCores() const = 0;
            void RemoveAll();
    };
}
