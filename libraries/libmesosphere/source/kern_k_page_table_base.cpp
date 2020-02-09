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
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern {

    Result KPageTableBase::InitializeForKernel(bool is_64_bit, void *table, KVirtualAddress start, KVirtualAddress end) {
        /* Initialize impl table. */
        this->impl.InitializeForKernel(table, start, end);

        /* Initialize our members. */
        MESOSPHERE_TODO("KPageTableBase member initialization");

        /* Initialize our memory block manager. */
        MESOSPHERE_TODO("R_TRY(this->memory_block_manager.Initialize(this->address_space_start, this->address_space_end, this->GetMemoryBlockSlabManager()));");

        return ResultSuccess();
    }

    void KPageTableBase::Finalize() {
        MESOSPHERE_TODO("this->memory_block_manager.Finalize(this->GetMemoryBlockSlabManager());");
        MESOSPHERE_TODO("cpu::InvalidateEntireInstructionCache();");
    }
}
