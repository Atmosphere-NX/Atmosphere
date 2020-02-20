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

    Result SetProcessMemoryPermission64(ams::svc::Handle process_handle, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcSetProcessMemoryPermission64 was called.");
    }

    Result MapProcessMemory64(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcMapProcessMemory64 was called.");
    }

    Result UnmapProcessMemory64(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapProcessMemory64 was called.");
    }

    Result MapProcessCodeMemory64(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcMapProcessCodeMemory64 was called.");
    }

    Result UnmapProcessCodeMemory64(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapProcessCodeMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetProcessMemoryPermission64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcSetProcessMemoryPermission64From32 was called.");
    }

    Result MapProcessMemory64From32(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcMapProcessMemory64From32 was called.");
    }

    Result UnmapProcessMemory64From32(ams::svc::Address dst_address, ams::svc::Handle process_handle, uint64_t src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapProcessMemory64From32 was called.");
    }

    Result MapProcessCodeMemory64From32(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcMapProcessCodeMemory64From32 was called.");
    }

    Result UnmapProcessCodeMemory64From32(ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapProcessCodeMemory64From32 was called.");
    }

}
