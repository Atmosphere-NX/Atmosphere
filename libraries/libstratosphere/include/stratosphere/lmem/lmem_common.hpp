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
#include <vapours.hpp>
#include <stratosphere/lmem/impl/lmem_impl_common.hpp>

namespace ams::lmem {

    enum CreateOption {
        CreateOption_None       = (0),
        CreateOption_ZeroClear  = (1 << 0),
        CreateOption_DebugFill  = (1 << 1),
        CreateOption_ThreadSafe = (1 << 2),
    };

    enum FillType {
        FillType_Unallocated,
        FillType_Allocated,
        FillType_Freed,
        FillType_Count,
    };

    namespace impl {

        struct HeapHead;

    }

    using HeapHandle = impl::HeapHead *;

    using HeapCommonHead = impl::HeapHead;

    struct MemoryRange {
        uintptr_t address;
        size_t size;
    };

    constexpr inline s32 DefaultAlignment = 0x8;

    /* Common API. */
    u32  GetDebugFillValue(FillType fill_type);
    void SetDebugFillValue(FillType fill_type, u32 value);

    size_t GetTotalSize(HeapHandle handle);
    void *GetStartAddress(HeapHandle handle);
    bool ContainsAddress(HeapHandle handle, const void *address);

}
