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

    Result SetMemoryPermission64(ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcSetMemoryPermission64 was called.");
    }

    Result SetMemoryAttribute64(ams::svc::Address address, ams::svc::Size size, uint32_t mask, uint32_t attr) {
        MESOSPHERE_PANIC("Stubbed SvcSetMemoryAttribute64 was called.");
    }

    Result MapMemory64(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcMapMemory64 was called.");
    }

    Result UnmapMemory64(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetMemoryPermission64From32(ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission perm) {
        MESOSPHERE_PANIC("Stubbed SvcSetMemoryPermission64From32 was called.");
    }

    Result SetMemoryAttribute64From32(ams::svc::Address address, ams::svc::Size size, uint32_t mask, uint32_t attr) {
        MESOSPHERE_PANIC("Stubbed SvcSetMemoryAttribute64From32 was called.");
    }

    Result MapMemory64From32(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcMapMemory64From32 was called.");
    }

    Result UnmapMemory64From32(ams::svc::Address dst_address, ams::svc::Address src_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapMemory64From32 was called.");
    }

}
