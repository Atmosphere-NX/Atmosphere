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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern::board::nintendo::nx {

    using KDeviceVirtualAddress = u64;

    class KDevicePageTable {
        private:
            static constexpr size_t TableCount = 4;
        private:
            KVirtualAddress tables[TableCount];
            u8 table_asids[TableCount];
            u64 attached_device;
            u32 attached_value;
            u32 detached_value;
            u32 hs_attached_value;
            u32 hs_detached_value;
        private:
            static ALWAYS_INLINE KVirtualAddress GetHeapVirtualAddress(KPhysicalAddress addr) {
                return KPageTable::GetHeapVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetHeapPhysicalAddress(KVirtualAddress addr) {
                return KPageTable::GetHeapPhysicalAddress(addr);
            }

            static ALWAYS_INLINE KVirtualAddress GetPageTableVirtualAddress(KPhysicalAddress addr) {
                return KPageTable::GetPageTableVirtualAddress(addr);
            }

            static ALWAYS_INLINE KPhysicalAddress GetPageTablePhysicalAddress(KVirtualAddress addr) {
                return KPageTable::GetPageTablePhysicalAddress(addr);
            }
        public:
            constexpr KDevicePageTable() : tables(), table_asids(), attached_device(), attached_value(), detached_value(), hs_attached_value(), hs_detached_value() { /* ... */ }

            Result Initialize(u64 space_address, u64 space_size);
            void Finalize();

            Result Attach(ams::svc::DeviceName device_name, u64 space_address, u64 space_size);
            Result Detach(ams::svc::DeviceName device_name);

            Result Map(size_t *out_mapped_size, const KPageGroup &pg, KDeviceVirtualAddress device_address, ams::svc::MemoryPermission device_perm, bool refresh_mappings);
            Result Unmap(const KPageGroup &pg, KDeviceVirtualAddress device_address);
        private:
            Result MapDevicePage(size_t *out_mapped_size, s32 *out_num_pt, s32 max_pt, KPhysicalAddress phys_addr, u64 size, KDeviceVirtualAddress address, ams::svc::MemoryPermission device_perm);

            Result MapImpl(size_t *out_mapped_size, s32 *out_num_pt, s32 max_pt, const KPageGroup &pg, KDeviceVirtualAddress device_address, ams::svc::MemoryPermission device_perm);
            void UnmapImpl(KDeviceVirtualAddress address, u64 size, bool force);

            bool IsFree(KDeviceVirtualAddress address, u64 size) const;
            Result MakePageGroup(KPageGroup *out, KDeviceVirtualAddress address, u64 size) const;
            bool Compare(const KPageGroup &pg, KDeviceVirtualAddress device_address) const;
        public:
            static void Initialize();
    };

}