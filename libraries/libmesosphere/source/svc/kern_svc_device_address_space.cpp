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

    Result CreateDeviceAddressSpace64(ams::svc::Handle *out_handle, uint64_t das_address, uint64_t das_size) {
        MESOSPHERE_PANIC("Stubbed SvcCreateDeviceAddressSpace64 was called.");
    }

    Result AttachDeviceAddressSpace64(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        MESOSPHERE_PANIC("Stubbed SvcAttachDeviceAddressSpace64 was called.");
    }

    Result DetachDeviceAddressSpace64(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        MESOSPHERE_PANIC("Stubbed SvcDetachDeviceAddressSpace64 was called.");
    }

    Result MapDeviceAddressSpaceByForce64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpaceByForce64 was called.");
    }

    Result MapDeviceAddressSpaceAligned64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpaceAligned64 was called.");
    }

    Result MapDeviceAddressSpace64(ams::svc::Size *out_mapped_size, ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpace64 was called.");
    }

    Result UnmapDeviceAddressSpace64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapDeviceAddressSpace64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateDeviceAddressSpace64From32(ams::svc::Handle *out_handle, uint64_t das_address, uint64_t das_size) {
        MESOSPHERE_PANIC("Stubbed SvcCreateDeviceAddressSpace64From32 was called.");
    }

    Result AttachDeviceAddressSpace64From32(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        MESOSPHERE_PANIC("Stubbed SvcAttachDeviceAddressSpace64From32 was called.");
    }

    Result DetachDeviceAddressSpace64From32(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        MESOSPHERE_PANIC("Stubbed SvcDetachDeviceAddressSpace64From32 was called.");
    }

    Result MapDeviceAddressSpaceByForce64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpaceByForce64From32 was called.");
    }

    Result MapDeviceAddressSpaceAligned64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpaceAligned64From32 was called.");
    }

    Result MapDeviceAddressSpace64From32(ams::svc::Size *out_mapped_size, ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, ams::svc::MemoryPermission device_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapDeviceAddressSpace64From32 was called.");
    }

    Result UnmapDeviceAddressSpace64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapDeviceAddressSpace64From32 was called.");
    }

}
