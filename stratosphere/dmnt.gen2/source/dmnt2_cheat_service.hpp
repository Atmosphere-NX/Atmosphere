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

/* TODO: In libstratosphere, eventually? */
#define AMS_DMNT_I_CHEAT_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H, 65102, Result, BreakPoint,      (const sf::OutBuffer &buffer, u64 address, u64 out_size),                                             (buffer, address, out_size))    \


AMS_SF_DEFINE_INTERFACE(ams::dmnt::cheat::impl, ICheatInterface, AMS_DMNT_I_CHEAT_INTERFACE_INTERFACE_INFO, 0x00000000)

namespace ams::dmnt::cheat {

    class CheatService {
        public:
            Result BreakPoint(const sf::OutBuffer &buffer, u64 address, u64 out_size);

    };
    static_assert(impl::IsICheatInterface<CheatService>);

}
