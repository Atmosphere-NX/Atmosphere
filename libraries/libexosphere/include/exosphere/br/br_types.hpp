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

#include <exosphere/br/impl/br_erista_types.hpp>
#include <exosphere/br/impl/br_mariko_types.hpp>

namespace ams::br {

    struct BootEcid {
        u32 ecid[4];
    };
    static_assert(util::is_pod<BootEcid>::value);
    static_assert(sizeof(BootEcid) == 0x10);

}
