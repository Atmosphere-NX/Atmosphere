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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_page_group.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern::board::generic {

    using KDeviceVirtualAddress = u64;

    class KDevicePageTable {
        public:
            constexpr KDevicePageTable() { /* ... */ }

            Result ALWAYS_INLINE Initialize(u64 space_address, u64 space_size) {
                MESOSPHERE_UNUSED(space_address, space_size);
                R_THROW(ams::kern::svc::ResultNotImplemented());
            }

            void ALWAYS_INLINE Finalize() { /* ... */ }

            Result ALWAYS_INLINE Attach(ams::svc::DeviceName device_name, u64 space_address, u64 space_size) {
                MESOSPHERE_UNUSED(device_name, space_address, space_size);
                R_THROW(ams::kern::svc::ResultNotImplemented());
            }

            Result ALWAYS_INLINE Detach(ams::svc::DeviceName device_name) {
                MESOSPHERE_UNUSED(device_name);
                R_THROW(ams::kern::svc::ResultNotImplemented());
            }

            Result ALWAYS_INLINE Map(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, KDeviceVirtualAddress device_address, ams::svc::MemoryPermission device_perm, bool is_aligned, bool is_io) {
                MESOSPHERE_UNUSED(page_table, process_address, size, device_address, device_perm, is_aligned, is_io);
                R_THROW(ams::kern::svc::ResultNotImplemented());
            }

            Result ALWAYS_INLINE Unmap(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, KDeviceVirtualAddress device_address) {
                MESOSPHERE_UNUSED(page_table, process_address, size, device_address);
                R_THROW(ams::kern::svc::ResultNotImplemented());
            }

            void ALWAYS_INLINE Unmap(KDeviceVirtualAddress device_address, size_t size) {
                MESOSPHERE_UNUSED(device_address, size);
            }
        public:
            static ALWAYS_INLINE void Initialize() { /* ... */ }

            static ALWAYS_INLINE void Lock() { /* ... */ }
            static ALWAYS_INLINE void Unlock() { /* ... */ }
            static ALWAYS_INLINE void Sleep() { /* ... */ }
            static ALWAYS_INLINE void Wakeup() { /* ... */ }
    };

}