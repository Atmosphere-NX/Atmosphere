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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    constexpr inline bool HasOverlap(uintptr_t addr0, size_t size0, uintptr_t addr1, size_t size1) {
        if (addr0 <= addr1) {
            return addr1 < addr0 + size0;
        } else {
            return addr0 < addr1 + size1;
        }
    }

    constexpr inline bool Contains(uintptr_t addr, size_t size, uintptr_t ptr) {
        return (addr <= ptr) && (ptr < addr + size);
    }

}
