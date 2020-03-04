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
#include "fs_common.hpp"
#include "fs_file.hpp"

namespace ams::fs {

    struct QueryRangeInfo {
        u32 aes_ctr_key_type;
        u32 speed_emulation_type;
        u32 reserved[0x38 / sizeof(u32)];

        void Clear() {
            this->aes_ctr_key_type = 0;
            this->speed_emulation_type = 0;
            std::memset(this->reserved, 0, sizeof(this->reserved));
        }

        void Merge(const QueryRangeInfo &rhs) {
            this->aes_ctr_key_type |= rhs.aes_ctr_key_type;
            this->speed_emulation_type |= rhs.speed_emulation_type;
        }
    };

    static_assert(std::is_pod<QueryRangeInfo>::value);
    static_assert(sizeof(QueryRangeInfo) == 0x40);
    static_assert(sizeof(QueryRangeInfo) == sizeof(::FsRangeInfo));

    using FileQueryRangeInfo    = QueryRangeInfo;
    using StorageQueryRangeInfo = QueryRangeInfo;

    Result QueryRange(QueryRangeInfo *out, FileHandle handle, s64 offset, s64 size);

}
