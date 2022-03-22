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

namespace ams::kern {

    class KProcess;
    class KIoPool;

    class KIoRegion final : public KAutoObjectWithSlabHeapAndContainer<KIoRegion, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KIoRegion, KAutoObject);
        private:
            friend class KProcess;
            friend class KIoPool;
        private:
            KLightLock m_lock;
            KIoPool *m_pool;
            KPhysicalAddress m_physical_address;
            size_t m_size;
            ams::svc::MemoryMapping m_mapping;
            ams::svc::MemoryPermission m_permission;
            bool m_is_initialized;
            bool m_is_mapped;
            util::IntrusiveListNode m_process_list_node;
            util::IntrusiveListNode m_pool_list_node;
        public:
            explicit KIoRegion() : m_pool(nullptr), m_is_initialized(false) { /* ... */ }

            Result Initialize(KIoPool *pool, KPhysicalAddress phys_addr, size_t size, ams::svc::MemoryMapping mapping, ams::svc::MemoryPermission perm);
            void Finalize();

            bool IsInitialized() const { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            Result Map(KProcessAddress address, size_t size, ams::svc::MemoryPermission map_perm);
            Result Unmap(KProcessAddress address, size_t size);

            bool Overlaps(KPhysicalAddress address, size_t size) const {
                return m_physical_address <= (address + size - 1) && address <= (m_physical_address + m_size - 1);
            }

            ALWAYS_INLINE KPhysicalAddress GetAddress() const { return m_physical_address; }
            ALWAYS_INLINE size_t GetSize() const { return m_size; }
    };

}
