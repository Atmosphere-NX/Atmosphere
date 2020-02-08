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

    Result CreateCodeMemory64(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcCreateCodeMemory64 was called.");
    }

    Result ControlCodeMemory64(ams::svc::Handle code_memory_handle, ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcControlCodeMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateCodeMemory64From32(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcCreateCodeMemory64From32 was called.");
    }

    Result ControlCodeMemory64From32(ams::svc::Handle code_memory_handle, ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcControlCodeMemory64From32 was called.");
    }

}
