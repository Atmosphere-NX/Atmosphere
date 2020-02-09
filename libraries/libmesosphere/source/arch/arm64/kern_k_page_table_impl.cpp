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
#include <mesosphere.hpp>

namespace ams::kern::arm64 {

    namespace {

        constexpr size_t PageBits  = __builtin_ctzll(PageSize);
        constexpr size_t NumLevels = 3;
        constexpr size_t LevelBits = 9;
        static_assert(NumLevels > 0);

        constexpr size_t AddressBits = (NumLevels - 1) * LevelBits + PageBits;
        static_assert(AddressBits <= BITSIZEOF(u64));
        constexpr size_t AddressSpaceSize = (1ull << AddressBits);

    }

    void KPageTableImpl::InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end) {
        this->table       = static_cast<u64 *>(tb);
        this->is_kernel   = true;
        this->num_entries = util::AlignUp(end - start, AddressSpaceSize) / AddressSpaceSize;
    }

    u64 *KPageTableImpl::Finalize() {
        return this->table;
    }

}
