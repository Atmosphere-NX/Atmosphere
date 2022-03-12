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

namespace ams::os::impl {

    class StackGuardManagerHorizonImpl {
        private:
            static u64 GetStackInfo(svc::InfoType type) {
                u64 value;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), type, svc::PseudoHandle::CurrentProcess, 0));
                AMS_ASSERT(value <= std::numeric_limits<size_t>::max());
                return static_cast<u64>(static_cast<size_t>(value));
            }
        public:
            static u64 GetStackGuardBeginAddress() { return GetStackInfo(svc::InfoType_StackRegionAddress); }
            static u64 GetStackGuardEndAddress() { return GetStackInfo(svc::InfoType_StackRegionSize); }
    };

    using StackGuardManagerImpl = StackGuardManagerHorizonImpl;

}
