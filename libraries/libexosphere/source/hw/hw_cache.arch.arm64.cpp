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
#include <exosphere.hpp>

namespace ams::hw::arch::arm64 {

    void FlushDataCache(const void *ptr, size_t size) {
        const uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t end   = util::AlignUp(start + size, hw::DataCacheLineSize);

        for (uintptr_t cur = start; cur < end; cur += hw::DataCacheLineSize) {
            FlushDataCacheLine(reinterpret_cast<void *>(cur));
        }
    }

}