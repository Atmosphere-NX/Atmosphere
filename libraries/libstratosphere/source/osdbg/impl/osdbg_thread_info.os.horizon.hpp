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
#include "osdbg_types.hpp"

namespace ams::osdbg::impl {

    class ThreadInfoHorizonImpl {
        public:
            static Result FillWithCurrentInfo(ThreadInfo *info);
    };

    using ThreadInfoImpl = ThreadInfoHorizonImpl;

    constexpr inline bool IsLp64(const ThreadInfo *info) {
        const auto as = info->_debug_info_create_process.flags & svc::CreateProcessFlag_AddressSpaceMask;
        return as == svc::CreateProcessFlag_AddressSpace64Bit || as == svc::CreateProcessFlag_AddressSpace64BitDeprecated;
    }

    constexpr inline bool Is64BitArch(const ThreadInfo *info) {
        return (info->_debug_info_create_process.flags & svc::CreateProcessFlag_Is64Bit);
    }

}
