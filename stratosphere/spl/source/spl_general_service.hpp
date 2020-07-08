/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::spl {

    class GeneralService {
        public:
            /* Actual commands. */
            Result GetConfig(sf::Out<u64> out, u32 which);
            Result ModularExponentiate(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod);
            Result SetConfig(u32 which, u64 value);
            Result GenerateRandomBytes(const sf::OutPointerBuffer &out);
            Result IsDevelopment(sf::Out<bool> is_dev);
            Result SetBootReason(BootReasonValue boot_reason);
            Result GetBootReason(sf::Out<BootReasonValue> out);
    };
    static_assert(spl::impl::IsIGeneralInterface<GeneralService>);

}
