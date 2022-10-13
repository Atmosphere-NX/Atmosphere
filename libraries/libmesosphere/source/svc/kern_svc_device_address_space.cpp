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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        constexpr inline u64 DeviceAddressSpaceAlignMask = (1ul << 22) - 1;

        constexpr bool IsProcessAndDeviceAligned(uint64_t process_address, uint64_t device_address) {
            return (process_address & DeviceAddressSpaceAlignMask) == (device_address & DeviceAddressSpaceAlignMask);
        }

        Result CreateDeviceAddressSpace(ams::svc::Handle *out, uint64_t das_address, uint64_t das_size) {
            /* Validate input. */
            R_UNLESS(util::IsAligned(das_address, PageSize), svc::ResultInvalidMemoryRegion());
            R_UNLESS(util::IsAligned(das_size, PageSize),    svc::ResultInvalidMemoryRegion());
            R_UNLESS(das_size > 0,                           svc::ResultInvalidMemoryRegion());
            R_UNLESS((das_address < das_address + das_size), svc::ResultInvalidMemoryRegion());

            /* Create the device address space. */
            KDeviceAddressSpace *das = KDeviceAddressSpace::Create();
            R_UNLESS(das != nullptr, svc::ResultOutOfResource());
            ON_SCOPE_EXIT { das->Close(); };

            /* Initialize the device address space. */
            R_TRY(das->Initialize(das_address, das_size));

            /* Register the device address space. */
            KDeviceAddressSpace::Register(das);

            /* Add to the handle table. */
            R_TRY(GetCurrentProcess().GetHandleTable().Add(out, das));

            R_SUCCEED();
        }

        Result AttachDeviceAddressSpace(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
            /* Get the device address space. */
            KScopedAutoObject das = GetCurrentProcess().GetHandleTable().GetObject<KDeviceAddressSpace>(das_handle);
            R_UNLESS(das.IsNotNull(), svc::ResultInvalidHandle());

            /* Attach. */
            R_RETURN(das->Attach(device_name));
        }

        Result DetachDeviceAddressSpace(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
            /* Get the device address space. */
            KScopedAutoObject das = GetCurrentProcess().GetHandleTable().GetObject<KDeviceAddressSpace>(das_handle);
            R_UNLESS(das.IsNotNull(), svc::ResultInvalidHandle());

            /* Detach. */
            R_RETURN(das->Detach(device_name));
        }

        constexpr bool IsValidDeviceMemoryPermission(ams::svc::MemoryPermission device_perm) {
            switch (device_perm) {
                case ams::svc::MemoryPermission_Read:
                case ams::svc::MemoryPermission_Write:
                case ams::svc::MemoryPermission_ReadWrite:
                    return true;
                default:
                    return false;
            }
        }

        Result MapDeviceAddressSpaceByForce(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, size_t size, uint64_t device_address, u32 option) {
            /* Decode the option. */
            const util::BitPack32 option_pack = { option };
            const auto device_perm = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Permission>();
            const auto reserved    = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Reserved>();

            /* Validate input. */
            R_UNLESS(util::IsAligned(process_address, PageSize),                   svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(device_address, PageSize),                    svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, PageSize),                              svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                                     svc::ResultInvalidSize());
            R_UNLESS((process_address < process_address + size),                   svc::ResultInvalidCurrentMemory());
            R_UNLESS((device_address < device_address + size),                     svc::ResultInvalidMemoryRegion());
            R_UNLESS((process_address == static_cast<uintptr_t>(process_address)), svc::ResultInvalidCurrentMemory());
            R_UNLESS(IsValidDeviceMemoryPermission(device_perm),                   svc::ResultInvalidNewMemoryPermission());
            R_UNLESS(reserved == 0,                                                svc::ResultInvalidEnumValue());

            /* Get the device address space. */
            KScopedAutoObject das = GetCurrentProcess().GetHandleTable().GetObject<KDeviceAddressSpace>(das_handle);
            R_UNLESS(das.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the process address is within range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(process_address, size), svc::ResultInvalidCurrentMemory());

            /* Map. */
            R_RETURN(das->MapByForce(std::addressof(page_table), KProcessAddress(process_address), size, device_address, option));
        }

        Result MapDeviceAddressSpaceAligned(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, size_t size, uint64_t device_address, u32 option) {
            /* Decode the option. */
            const util::BitPack32 option_pack = { option };
            const auto device_perm = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Permission>();
            const auto reserved    = option_pack.Get<ams::svc::MapDeviceAddressSpaceOption::Reserved>();

            /* Validate input. */
            R_UNLESS(util::IsAligned(process_address, PageSize),                   svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(device_address, PageSize),                    svc::ResultInvalidAddress());
            R_UNLESS(IsProcessAndDeviceAligned(process_address, device_address),   svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, PageSize),                              svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                                     svc::ResultInvalidSize());
            R_UNLESS((process_address < process_address + size),                   svc::ResultInvalidCurrentMemory());
            R_UNLESS((device_address < device_address + size),                     svc::ResultInvalidMemoryRegion());
            R_UNLESS((process_address == static_cast<uintptr_t>(process_address)), svc::ResultInvalidCurrentMemory());
            R_UNLESS(IsValidDeviceMemoryPermission(device_perm),                   svc::ResultInvalidNewMemoryPermission());
            R_UNLESS(reserved == 0,                                                svc::ResultInvalidEnumValue());

            /* Get the device address space. */
            KScopedAutoObject das = GetCurrentProcess().GetHandleTable().GetObject<KDeviceAddressSpace>(das_handle);
            R_UNLESS(das.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the process address is within range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(process_address, size), svc::ResultInvalidCurrentMemory());

            /* Map. */
            R_RETURN(das->MapAligned(std::addressof(page_table), KProcessAddress(process_address), size, device_address, option));
        }

        Result UnmapDeviceAddressSpace(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, size_t size, uint64_t device_address) {
            /* Validate input. */
            R_UNLESS(util::IsAligned(process_address, PageSize),                   svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(device_address, PageSize),                    svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size, PageSize),                              svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                                     svc::ResultInvalidSize());
            R_UNLESS((process_address < process_address + size),                   svc::ResultInvalidCurrentMemory());
            R_UNLESS((device_address < device_address + size),                     svc::ResultInvalidMemoryRegion());
            R_UNLESS((process_address == static_cast<uintptr_t>(process_address)), svc::ResultInvalidCurrentMemory());

            /* Get the device address space. */
            KScopedAutoObject das = GetCurrentProcess().GetHandleTable().GetObject<KDeviceAddressSpace>(das_handle);
            R_UNLESS(das.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Validate that the process address is within range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(process_address, size), svc::ResultInvalidCurrentMemory());

            R_RETURN(das->Unmap(std::addressof(page_table), KProcessAddress(process_address), size, device_address));
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateDeviceAddressSpace64(ams::svc::Handle *out_handle, uint64_t das_address, uint64_t das_size) {
        R_RETURN(CreateDeviceAddressSpace(out_handle, das_address, das_size));
    }

    Result AttachDeviceAddressSpace64(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        R_RETURN(AttachDeviceAddressSpace(device_name, das_handle));
    }

    Result DetachDeviceAddressSpace64(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        R_RETURN(DetachDeviceAddressSpace(device_name, das_handle));
    }

    Result MapDeviceAddressSpaceByForce64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, u32 option) {
        R_RETURN(MapDeviceAddressSpaceByForce(das_handle, process_handle, process_address, size, device_address, option));
    }

    Result MapDeviceAddressSpaceAligned64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, u32 option) {
        R_RETURN(MapDeviceAddressSpaceAligned(das_handle, process_handle, process_address, size, device_address, option));
    }

    Result UnmapDeviceAddressSpace64(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address) {
        R_RETURN(UnmapDeviceAddressSpace(das_handle, process_handle, process_address, size, device_address));
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateDeviceAddressSpace64From32(ams::svc::Handle *out_handle, uint64_t das_address, uint64_t das_size) {
        R_RETURN(CreateDeviceAddressSpace(out_handle, das_address, das_size));
    }

    Result AttachDeviceAddressSpace64From32(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        R_RETURN(AttachDeviceAddressSpace(device_name, das_handle));
    }

    Result DetachDeviceAddressSpace64From32(ams::svc::DeviceName device_name, ams::svc::Handle das_handle) {
        R_RETURN(DetachDeviceAddressSpace(device_name, das_handle));
    }

    Result MapDeviceAddressSpaceByForce64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, u32 option) {
        R_RETURN(MapDeviceAddressSpaceByForce(das_handle, process_handle, process_address, size, device_address, option));
    }

    Result MapDeviceAddressSpaceAligned64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address, u32 option) {
        R_RETURN(MapDeviceAddressSpaceAligned(das_handle, process_handle, process_address, size, device_address, option));
    }

    Result UnmapDeviceAddressSpace64From32(ams::svc::Handle das_handle, ams::svc::Handle process_handle, uint64_t process_address, ams::svc::Size size, uint64_t device_address) {
        R_RETURN(UnmapDeviceAddressSpace(das_handle, process_handle, process_address, size, device_address));
    }

}
