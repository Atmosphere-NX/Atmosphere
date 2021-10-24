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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_select_device_page_table.hpp>

namespace ams::kern {

    class KDeviceAddressSpace final : public KAutoObjectWithSlabHeapAndContainer<KDeviceAddressSpace, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KDeviceAddressSpace, KAutoObject);
        private:
            KLightLock m_lock;
            KDevicePageTable m_table;
            u64 m_space_address;
            u64 m_space_size;
            bool m_is_initialized;
        public:
            explicit KDeviceAddressSpace() : m_is_initialized(false) { /* ... */ }

            Result Initialize(u64 address, u64 size);
            void Finalize();

            bool IsInitialized() const { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            Result Attach(ams::svc::DeviceName device_name);
            Result Detach(ams::svc::DeviceName device_name);

            Result MapByForce(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, ams::svc::MemoryPermission device_perm) {
                return this->Map(page_table, process_address, size, device_address, device_perm, false);
            }

            Result MapAligned(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, ams::svc::MemoryPermission device_perm) {
                return this->Map(page_table, process_address, size, device_address, device_perm, true);
            }

            Result Unmap(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address);
        private:
            Result Map(KProcessPageTable *page_table, KProcessAddress process_address, size_t size, u64 device_address, ams::svc::MemoryPermission device_perm, bool is_aligned);
        public:
            static void Initialize();
    };

}
