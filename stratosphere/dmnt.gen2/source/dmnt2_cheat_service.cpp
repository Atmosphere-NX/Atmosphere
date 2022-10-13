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
#include <stratosphere.hpp>
#include "dmnt2_cheat_service.hpp"
#include "dmnt2_cheat_api.hpp"

namespace ams::dmnt::cheat {

    

    Result CheatService::BreakPoint(const sf::OutBuffer &buffer, u64 address, u64 out_size) {
        R_UNLESS(buffer.GetPointer() != nullptr, dmnt::cheat::ResultCheatNullBuffer());
        R_RETURN(dmnt::cheat::impl::BreakPoint(address, buffer.GetPointer(), std::min(out_size, buffer.GetSize())));
    }

    

}
