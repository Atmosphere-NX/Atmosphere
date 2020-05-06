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
#include <errno.h>

namespace ams::mem::impl {

    constexpr inline size_t MaxSize = static_cast<size_t>(std::numeric_limits<s64>::max());

    using errno_t = int;

    enum DumpMode {
        DumpMode_Basic    = (0 << 0),
        DumpMode_Spans    = (1 << 0),
        DumpMode_Pointers = (1 << 1),
        DumpMode_Pages    = (1 << 2),
        DumpMode_All      = (DumpMode_Pages | DumpMode_Pointers | DumpMode_Spans | DumpMode_Basic),
    };

    enum AllocQuery {
        AllocQuery_Dump                     =  0,
        AllocQuery_PageSize                 =  1,
        AllocQuery_AllocatedSize            =  2,
        AllocQuery_FreeSize                 =  3,
        AllocQuery_SystemSize               =  4,
        AllocQuery_MaxAllocatableSize       =  5,
        AllocQuery_IsClean                  =  6,
        AllocQuery_HeapHash                 =  7,
        AllocQuery_UnifyFreeList            =  8,
        AllocQuery_SetColor                 =  9,
        AllocQuery_GetColor                 = 10,
        AllocQuery_SetName                  = 11,
        AllocQuery_GetName                  = 12,
        /* AllocQuery_Thirteen                 = 13, */
        AllocQuery_CheckCache               = 14,
        AllocQuery_ClearCache               = 15,
        AllocQuery_FinalizeCache            = 16,
        AllocQuery_FreeSizeMapped           = 17,
        AllocQuery_MaxAllocatableSizeMapped = 18,
        AllocQuery_DumpJson                 = 19,
    };

    enum HeapOption {
        HeapOption_UseEnvironment = (1 << 0),
        HeapOption_DisableCache   = (1 << 2),
    };

    struct HeapHash {
        size_t alloc_count;
        size_t alloc_size;
        size_t hash;
    };
    static_assert(util::is_pod<HeapHash>::value);

}
