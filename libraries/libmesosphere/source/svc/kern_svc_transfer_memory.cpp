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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    Result MapTransferMemory64From32(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission owner_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapTransferMemory64From32 was called.");
    }

    Result UnmapTransferMemory64From32(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapTransferMemory64From32 was called.");
    }

    Result CreateTransferMemory64(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_PANIC("Stubbed SvcCreateTransferMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result MapTransferMemory64(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission owner_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapTransferMemory64 was called.");
    }

    Result UnmapTransferMemory64(ams::svc::Handle trmem_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapTransferMemory64 was called.");
    }

    Result CreateTransferMemory64From32(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_PANIC("Stubbed SvcCreateTransferMemory64From32 was called.");
    }

}
