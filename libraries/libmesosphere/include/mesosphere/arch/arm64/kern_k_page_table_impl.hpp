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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>

namespace ams::kern::arm64 {

    /* TODO: This seems worse than KInitialPageTable. Can we fulfill Nintendo's API using KInitialPageTable? */
    /* KInitialPageTable is significantly nicer, but doesn't have KPageTableImpl's traversal semantics. */
    /* Perhaps we could implement those on top of it? */
    class KPageTableImpl {
        NON_COPYABLE(KPageTableImpl);
        NON_MOVEABLE(KPageTableImpl);
        private:
            u64 *table;
            bool is_kernel;
            u32  num_entries;
        public:
            constexpr KPageTableImpl() : table(), is_kernel(), num_entries() { /* ... */ }

            void InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end);

            u64 *Finalize();
    };

}
