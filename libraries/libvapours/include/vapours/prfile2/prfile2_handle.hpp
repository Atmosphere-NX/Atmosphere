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
#include <vapours/prfile2/prfile2_common.hpp>

namespace ams::prfile2 {

    using HandleType = util::BitPack32;

    struct HandleField {
        using Id        = util::BitPack32::Field< 0,  8>;
        using Kind      = util::BitPack32::Field< 8,  8>;
        using Signature = util::BitPack32::Field<16, 16>;
    };

    enum HandleKind {
        HandleKind_Disk      = 3,
        HandleKind_Partition = 4,
    };

    constexpr ALWAYS_INLINE HandleType ConstructHandle(u8 id, HandleKind kind, u16 signature) {
        /* Construct the handle. */
        HandleType val = {};
        val.Set<HandleField::Id>(id);
        val.Set<HandleField::Kind>(kind);
        val.Set<HandleField::Signature>(signature);
        return val;
    };

    constexpr ALWAYS_INLINE u8 GetHandleId(HandleType handle) {
        return handle.Get<HandleField::Id>();
    }

    constexpr ALWAYS_INLINE u16 GetHandleSignature(HandleType handle) {
        return handle.Get<HandleField::Signature>();
    }

    constexpr inline const HandleType InvalidHandle = {};

    constexpr ALWAYS_INLINE bool IsInvalidHandle(HandleType handle) {
        return handle.value == 0;
    }
    static_assert(IsInvalidHandle(InvalidHandle));

    constexpr ALWAYS_INLINE HandleType ConstructDiskHandle(u8 id, u16 signature) { return ConstructHandle(id, HandleKind_Disk, signature); }
    constexpr ALWAYS_INLINE HandleType ConstructPartitionHandle(u8 id, u16 signature) { return ConstructHandle(id, HandleKind_Partition, signature); }

    constexpr ALWAYS_INLINE bool IsDiskHandle(HandleType handle) { return handle.Get<HandleField::Kind>() == HandleKind_Disk; }
    constexpr ALWAYS_INLINE bool IsPartitionHandle(HandleType handle) { return handle.Get<HandleField::Kind>() == HandleKind_Partition; }

}
