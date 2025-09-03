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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "secmon_smc_register_access.hpp"

namespace ams::secmon::smc {

    namespace {

        template<size_t N>
        constexpr void SetRegisterTableAllowed(std::array<u8, N> &arr, uintptr_t reg) {
            /* All registers should be four byte aligned. */
            AMS_ASSUME(reg % sizeof(u32) == 0);

            /* Reduce the register to an index. */
            reg /= sizeof(u32);

            /* Get the index and mask. */
            const auto index = reg / BITSIZEOF(u8);
            const auto mask  = (1u << (reg % BITSIZEOF(u8)));

            /* Check that the permission bit isn't already set. */
            AMS_ASSUME((arr[index] & mask) == 0);

            /* Set the permission bit. */
            arr[index] |= mask;

            /* Ensure that indices are set in sorted order. */
            for (auto i = (reg % BITSIZEOF(u8)) + 1; i < 8; ++i) {
                AMS_ASSUME((arr[index] & (1u << i)) == 0);
            }

            for (auto i = index + 1; i < arr.size(); ++i) {
                AMS_ASSUME(arr[i] == 0);
            }
        }

        template<size_t N>
        consteval std::pair<size_t, size_t> GetReducedAccessTableInfo(const std::array<u8, N> &arr) {
            for (int last = arr.size() - 1; last >= 0; --last) {
                if (arr[last] != 0) {
                    const int end = last + 1;
                    for (int start = 0; start < end; ++start) {
                        if (arr[start] != 0) {
                            return std::make_pair(static_cast<size_t>(start), static_cast<size_t>(end));
                        }
                    }
                    return std::make_pair(static_cast<size_t>(0), static_cast<size_t>(end));
                }
            }

            /* All empty perm table is disallowed. */
            AMS_ASSUME(false);
        }


        template<u32 _Address, auto RawTable>
        struct AccessTable {
            static constexpr inline auto   ReducedAccessTableInfo = GetReducedAccessTableInfo(RawTable);
            static constexpr inline size_t ReducedAccessTableSize = ReducedAccessTableInfo.second - ReducedAccessTableInfo.first;
            static constexpr inline auto   ReducedAccessTable     = []() -> std::array<u8, ReducedAccessTableSize> {
                std::array<u8, ReducedAccessTableSize> reduced = {};

                for (size_t i = ReducedAccessTableInfo.first; i < ReducedAccessTableInfo.second; ++i) {
                    reduced[i - ReducedAccessTableInfo.first] = RawTable[i];
                }

                return reduced;
            }();

            static constexpr u32 Address = _Address + (ReducedAccessTableInfo.first * sizeof(u32) * BITSIZEOF(u8));
            static constexpr u32 Size    = static_cast<u32>(ReducedAccessTableSize * sizeof(u32) * BITSIZEOF(u8));

            static_assert(Size <= 0x1000);
        };

        struct AccessTableEntry {
            const u8 * const table;
            uintptr_t virtual_address;
            u32 address;
            u32 size;
        };

        /* Include the access tables. */
        #include "secmon_define_pmc_access_table.inc"
        #include "secmon_define_mc_access_table.inc"
        #include "secmon_define_mc01_access_table.inc"

        constexpr const AccessTableEntry AccessTables[] = {
            {  PmcAccessTable::ReducedAccessTable.data(),                            MemoryRegionVirtualDevicePmc.GetAddress(),                PmcAccessTable::Address,                                                             PmcAccessTable::Size, },
            {   McAccessTable::ReducedAccessTable.data(),                            MemoryRegionVirtualDeviceMemoryController.GetAddress(),    McAccessTable::Address,                                                              McAccessTable::Size, },
            { Mc01AccessTable::ReducedAccessTable.data(), Mc01AccessTable::Address + MemoryRegionVirtualDeviceMemoryController0.GetAddress(), Mc01AccessTable::Address + MemoryRegionPhysicalDeviceMemoryController0.GetAddress(), Mc01AccessTable::Size, },
            { Mc01AccessTable::ReducedAccessTable.data(), Mc01AccessTable::Address + MemoryRegionVirtualDeviceMemoryController1.GetAddress(), Mc01AccessTable::Address + MemoryRegionPhysicalDeviceMemoryController1.GetAddress(), Mc01AccessTable::Size, },
        };

        constexpr bool IsAccessAllowed(const AccessTableEntry &entry, uintptr_t address) {
            /* Check if the address is within range. */
            if (!(entry.address <= address && address < entry.address + entry.size)) {
                return false;
            }

            /* Get the offset. */
            const auto offset = address - entry.address;

            /* Convert it to an index. */
            const auto reg_index = offset / sizeof(u32);

            /* Get the bit fields. */
            const auto index = reg_index / BITSIZEOF(u8);
            const auto mask  = (1u << (reg_index % BITSIZEOF(u8)));

            /* Validate that we're not going out of bounds. */
            if (index >= entry.size / sizeof(u32)) {
                return false;
            }

            return (entry.table[index] & mask) != 0;
        }

        constexpr const AccessTableEntry *GetAccessTableEntry(uintptr_t address) {
            for (const auto &entry : AccessTables) {
                if (IsAccessAllowed(entry, address)) {
                    return std::addressof(entry);
                }
            }

            return nullptr;
        }

    }

    SmcResult SmcReadWriteRegister(SmcArguments &args) {
        /* Get the arguments. */
        const uintptr_t address = args.r[1];
        const u32       mask    = args.r[2];
        const u32       value   = args.r[3];

        /* Validate that the address is aligned. */
        SMC_R_UNLESS(util::IsAligned(address, alignof(u32)), InvalidArgument);

        /* Find the access table. */
        const AccessTableEntry * const entry = GetAccessTableEntry(address);

        /* Translate our entry into an address to access. */
        uintptr_t virtual_address = 0;
        if (entry != nullptr) {
            /* Get the address to read or write. */
            virtual_address = entry->virtual_address + (address - entry->address);
        } else {
            /* For no clearly discernable reason, SmcReadWriteRegister returns success despite not doing the read/write */
            /* when accessing the SMMU controls for the BPMP and for APB-DMA. */
            /* This is "probably" to fuck with hackers who got access to the SMC and are trying to get control of the */
            /* BPMP to exploit jamais vu, deja vu, or other related DMA/wake-from-sleep vulnerabilities. */
            constexpr uintptr_t MC = MemoryRegionPhysicalDeviceMemoryController.GetAddress();
            SMC_R_UNLESS((address == (MC + MC_SMMU_AVPC_ASID) || address == (MC + MC_SMMU_PPCS1_ASID)), InvalidArgument);

            /* For backwards compatibility, we'll allow access to these devices on 1.0.0. */
            if (GetTargetFirmware() < TargetFirmware_2_0_0) {
                virtual_address = MemoryRegionVirtualDeviceMemoryController.GetAddress() + (address - MC);
            }
        }

        /* Perform the read or write, if we should. */
        if (virtual_address != 0) {
            u32 out = 0;

            if (mask != ~static_cast<u32>(0)) {
                out = reg::Read(virtual_address);
            }
            if (mask != static_cast<u32>(0)) {
                reg::Write(virtual_address, (out & ~mask) | (value & mask));
            }

            args.r[1] = out;
        }

        return SmcResult::Success;
    }

}
