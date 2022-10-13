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
#include <stratosphere.hpp>
#include "os_process_code_memory_impl.hpp"
#include "os_aslr_space_manager.hpp"

namespace ams::os::impl {

    namespace {

        class ProcessAddressSpaceAllocator final : public AddressSpaceAllocatorBase<u64, u64> {
            private:
                using Base = AddressSpaceAllocatorBase<u64, u64>;
            private:
                NativeHandle m_handle;
            public:
                ProcessAddressSpaceAllocator(u64 start_address, u64 end_address, SizeType guard_size, const AddressSpaceAllocatorForbiddenRegion *forbidden_regions, size_t num_forbidden_regions, NativeHandle handle) : Base(start_address, end_address, guard_size, forbidden_regions, num_forbidden_regions), m_handle(handle) {
                    /* ... */
                }
            public:
                virtual bool CheckFreeSpace(AddressType address, SizeType size) override {
                    /* Query the memory. */
                    svc::MemoryInfo memory_info;
                    svc::PageInfo page_info;
                    R_ABORT_UNLESS(svc::QueryProcessMemory(std::addressof(memory_info), std::addressof(page_info), m_handle, address));

                    return memory_info.state == svc::MemoryState_Free && address + size <= memory_info.base_address + memory_info.size;
                }
        };

        using ProcessAslrSpaceManager = AslrSpaceManagerTemplate<ProcessAddressSpaceAllocator, AslrSpaceManagerImpl>;

        size_t GetTotalProcessMemoryRegionSize(const ProcessMemoryRegion *regions, size_t num_regions) {
            size_t total = 0;

            for (size_t i = 0; i < num_regions; ++i) {
                total += regions[i].size;
            }

            return total;
        }

    }

    Result ProcessCodeMemoryImpl::Map(u64 *out, NativeHandle handle, const ProcessMemoryRegion *regions, size_t num_regions, AddressSpaceGenerateRandomFunction generate_random) {
        /* Get the total process memory region size. */
        const size_t total_size = GetTotalProcessMemoryRegionSize(regions, num_regions);

        /* Create an aslr space manager for the process. */
        auto process_aslr_space_manager = ProcessAslrSpaceManager(handle);

        /* Map at a random address. */
        u64 mapped_address;
        R_TRY(process_aslr_space_manager.MapAtRandomAddressWithCustomRandomGenerator(std::addressof(mapped_address),
            [handle, regions, num_regions](u64 map_address, u64 map_size) -> Result {
                AMS_UNUSED(map_size);

                /* Map the regions in order. */
                u64 mapped_size = 0;
                for (size_t i = 0; i < num_regions; ++i) {
                    /* If we fail, unmap up to where we've mapped. */
                    ON_RESULT_FAILURE { R_ABORT_UNLESS(ProcessCodeMemoryImpl::Unmap(handle, map_address, regions, i)); };

                    /* Map the current region. */
                    R_TRY_CATCH(svc::MapProcessCodeMemory(handle, map_address + mapped_size, regions[i].address, regions[i].size)) {
                        R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfResource())
                        R_CATCH(svc::ResultInvalidCurrentMemory) {
                            /* Check if the process memory is invalid. */
                            const u64 last_address = regions[i].address + regions[i].size - 1;
                            u64 cur_address = regions[i].address;
                            while (cur_address <= last_address) {
                                svc::MemoryInfo memory_info;
                                svc::PageInfo page_info;
                                R_ABORT_UNLESS(svc::QueryProcessMemory(std::addressof(memory_info), std::addressof(page_info), handle, cur_address));

                                R_UNLESS(memory_info.state == svc::MemoryState_Normal,                  os::ResultInvalidProcessMemory());
                                R_UNLESS(memory_info.permission == svc::MemoryPermission_ReadWrite,     os::ResultInvalidProcessMemory());
                                R_UNLESS(memory_info.attribute == static_cast<svc::MemoryAttribute>(0), os::ResultInvalidProcessMemory());

                                cur_address = memory_info.base_address + memory_info.size;
                            }

                            R_THROW(os::ResultInvalidCurrentMemoryState());
                        }
                    } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                    mapped_size += regions[i].size;
                }

                R_SUCCEED();
            },
            [handle, regions, num_regions](u64 map_address, u64 map_size) -> void {
                AMS_UNUSED(map_size);
                R_ABORT_UNLESS(ProcessCodeMemoryImpl::Unmap(handle, map_address, regions, num_regions));
            },
            total_size,
            regions[0].address, /* NOTE: This seems like a Nintendo bug, if the caller passed no regions. */
            generate_random
        ));

        /* Set the output address. */
        *out = mapped_address;
        R_SUCCEED();
    }

    Result ProcessCodeMemoryImpl::Unmap(NativeHandle handle, u64 process_code_address, const ProcessMemoryRegion *regions, size_t num_regions) {
        /* Get the total process memory region size. */
        const size_t total_size = GetTotalProcessMemoryRegionSize(regions, num_regions);

        /* Unmap each region in order. */
        size_t cur_offset = total_size;
        for (size_t i = 0; i < num_regions; ++i) {
            /* We want to unmap in reverse order. */
            const auto &cur_region = regions[num_regions - 1 - i];

            /* Subtract to update the current offset. */
            cur_offset -= cur_region.size;

            /* Unmap. */
            R_TRY_CATCH(svc::UnmapProcessCodeMemory(handle, process_code_address + cur_offset, cur_region.address, cur_region.size)) {
                R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
                R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultInvalidCurrentMemoryState())
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
        }

        R_SUCCEED();
    }

}
