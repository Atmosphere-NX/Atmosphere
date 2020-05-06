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
#include <vapours.hpp>
#include "../sf/sf_buffer_tags.hpp"

namespace ams::settings::fwdbg {

    constexpr size_t SettingsNameLengthMax = 0x40;
    constexpr size_t SettingsItemKeyLengthMax = 0x40;

    struct SettingsName : sf::LargeData {
        char value[util::AlignUp(SettingsNameLengthMax + 1, alignof(u64))];
    };

    static_assert(util::is_pod<SettingsName>::value && sizeof(SettingsName) > SettingsNameLengthMax);

    struct SettingsItemKey : sf::LargeData {
        char value[util::AlignUp(SettingsItemKeyLengthMax + 1, alignof(u64))];
    };

    static_assert(util::is_pod<SettingsItemKey>::value && sizeof(SettingsItemKey) > SettingsItemKeyLengthMax);

}
