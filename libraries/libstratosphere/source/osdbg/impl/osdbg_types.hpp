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
#include <stratosphere.hpp>

namespace ams::osdbg::impl {

    template<size_t Size, int NumPointers, size_t Alignment>
    using AlignedStorageIlp32 = typename std::aligned_storage<Size + NumPointers * sizeof(u32), Alignment>::type;

    template<size_t Size, int NumPointers, size_t Alignment>
    using AlignedStorageLp64 = typename std::aligned_storage<Size + NumPointers * sizeof(u64), Alignment>::type;

}
