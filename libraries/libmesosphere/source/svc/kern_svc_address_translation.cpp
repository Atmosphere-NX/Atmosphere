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

        Result QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out_info, uintptr_t address) {
            /* NOTE: In 10.0.0, Nintendo stubbed this SVC. Should we do so? */
            /* R_UNLESS(GetTargetFirmware() < TargetFirmware_10_0_0, svc::ResultInvalidCurrentMemory()); */

            /* Get reference to page table. */
            auto &pt = GetCurrentProcess().GetPageTable();

            /* Check that the address is valid. */
            R_UNLESS(pt.Contains(address, 1), svc::ResultInvalidCurrentMemory());

            /* Query the physical mapping. */
            R_TRY(pt.QueryPhysicalAddress(out_info, address));

            return ResultSuccess();
        }

        Result QueryIoMapping(uintptr_t *out_address, size_t *out_size, uint64_t phys_addr, size_t size) {
            /* Declare variables we'll populate. */
            KProcessAddress found_address = Null<KProcessAddress>;
            size_t          found_size    = 0;

            /* Get reference to page table. */
            auto &pt = GetCurrentProcess().GetPageTable();

            /* Check whether the address is aligned. */
            const bool aligned = util::IsAligned(phys_addr, PageSize);

            auto QueryIoMappingFromPageTable = [&] ALWAYS_INLINE_LAMBDA (uint64_t phys_addr, size_t size) -> Result {
                /* The size must be non-zero. */
                R_UNLESS(size > 0, svc::ResultInvalidSize());

                /* The request must not overflow. */
                R_UNLESS((phys_addr < phys_addr + size), svc::ResultNotFound());

                /* Query the mapping. */
                R_TRY(pt.QueryIoMapping(std::addressof(found_address), phys_addr, size));

                /* Use the size as the found size. */
                found_size = size;

                return ResultSuccess();
            };

            if (aligned) {
                /* Query the input. */
                R_TRY(QueryIoMappingFromPageTable(phys_addr, size));
            } else {
                if (kern::GetTargetFirmware() < TargetFirmware_8_0_0 && phys_addr >= PageSize) {
                    /* Query the aligned-down page. */
                    const size_t offset = phys_addr & (PageSize - 1);
                    R_TRY(QueryIoMappingFromPageTable(phys_addr - offset, size + offset));

                    /* Adjust the output address. */
                    found_address += offset;
                } else {
                    /* Newer kernel only allows unaligned addresses when they're special enum members. */
                    R_UNLESS(phys_addr < PageSize, svc::ResultNotFound());

                    /* Try to find the memory region. */
                    const KMemoryRegion * const region = [] ALWAYS_INLINE_LAMBDA (ams::svc::MemoryRegionType type) -> const KMemoryRegion * {
                        switch (type) {
                            case ams::svc::MemoryRegionType_KernelTraceBuffer: return KMemoryLayout::GetPhysicalKernelTraceBufferRegion();
                            case ams::svc::MemoryRegionType_OnMemoryBootImage: return KMemoryLayout::GetPhysicalOnMemoryBootImageRegion();
                            case ams::svc::MemoryRegionType_DTB:               return KMemoryLayout::GetPhysicalDTBRegion();
                            default:                                           return nullptr;
                        }
                    }(static_cast<ams::svc::MemoryRegionType>(phys_addr));

                    /* Ensure that we found the region. */
                    R_UNLESS(region != nullptr, svc::ResultNotFound());

                    /* Chcek that the region is valid. */
                    MESOSPHERE_ABORT_UNLESS(region->GetEndAddress() != 0);

                    R_TRY(pt.QueryStaticMapping(std::addressof(found_address), region->GetAddress(), region->GetSize()));
                    found_size = region->GetSize();
                }
            }

            /* We succeeded. */
            MESOSPHERE_ASSERT(found_address != Null<KProcessAddress>);
            MESOSPHERE_ASSERT(found_size    != 0);
            if (out_address != nullptr) {
                *out_address = GetInteger(found_address);
            }
            if (out_size != nullptr) {
                *out_size = found_size;
            }
            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result QueryPhysicalAddress64(ams::svc::lp64::PhysicalMemoryInfo *out_info, ams::svc::Address address) {
        return QueryPhysicalAddress(out_info, address);
    }

    Result QueryIoMapping64(ams::svc::Address *out_address, ams::svc::Size *out_size, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        static_assert(sizeof(*out_size)    == sizeof(size_t));
        return QueryIoMapping(reinterpret_cast<uintptr_t *>(out_address), reinterpret_cast<size_t *>(out_size), physical_address, size);
    }

    Result LegacyQueryIoMapping64(ams::svc::Address *out_address, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        return QueryIoMapping(reinterpret_cast<uintptr_t *>(out_address), nullptr, physical_address, size);
    }

    /* ============================= 64From32 ABI ============================= */

    Result QueryPhysicalAddress64From32(ams::svc::ilp32::PhysicalMemoryInfo *out_info, ams::svc::Address address) {
        ams::svc::PhysicalMemoryInfo info = {};
        R_TRY(QueryPhysicalAddress(std::addressof(info), address));

        *out_info = {
            .physical_address = info.physical_address,
            .virtual_address  = static_cast<u32>(info.virtual_address),
            .size             = static_cast<u32>(info.size),
        };
        return ResultSuccess();
    }

    Result QueryIoMapping64From32(ams::svc::Address *out_address, ams::svc::Size *out_size, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        static_assert(sizeof(*out_size)    == sizeof(size_t));
        return QueryIoMapping(reinterpret_cast<uintptr_t *>(out_address), reinterpret_cast<size_t *>(out_size), physical_address, size);
    }

    Result LegacyQueryIoMapping64From32(ams::svc::Address *out_address, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        static_assert(sizeof(*out_address) == sizeof(uintptr_t));
        return QueryIoMapping(reinterpret_cast<uintptr_t *>(out_address), nullptr, physical_address, size);
    }

}
