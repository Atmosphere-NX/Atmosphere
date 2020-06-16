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

namespace ams::fs::impl {

    constexpr inline size_t FilePathHashSize = 4;

    struct FilePathHash : public Newable {
        u8 data[FilePathHashSize];
    };
    static_assert(util::is_pod<FilePathHash>::value);

    inline bool operator==(const FilePathHash &lhs, const FilePathHash &rhs) {
        return std::memcmp(lhs.data, rhs.data, FilePathHashSize) == 0;
    }

    inline bool operator!=(const FilePathHash &lhs, const FilePathHash &rhs) {
        return !(lhs == rhs);
    }

}
