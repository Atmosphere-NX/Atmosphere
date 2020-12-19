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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern {

    class KProcess;
    class KResourceLimit;

    class KSharedMemory final : public KAutoObjectWithSlabHeapAndContainer<KSharedMemory, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSharedMemory, KAutoObject);
        private:
            KPageGroup m_page_group;
            KResourceLimit *m_resource_limit;
            u64 m_owner_process_id;
            ams::svc::MemoryPermission m_owner_perm;
            ams::svc::MemoryPermission m_remote_perm;
            bool m_is_initialized;
        public:
            explicit KSharedMemory()
                : m_page_group(std::addressof(Kernel::GetBlockInfoManager())), m_resource_limit(nullptr), m_owner_process_id(std::numeric_limits<u64>::max()),
                  m_owner_perm(ams::svc::MemoryPermission_None), m_remote_perm(ams::svc::MemoryPermission_None), m_is_initialized(false)
            {
                /* ... */
            }

            virtual ~KSharedMemory() { /* ... */ }

            Result Initialize(KProcess *owner, size_t size, ams::svc::MemoryPermission own_perm, ams::svc::MemoryPermission rem_perm);
            virtual void Finalize() override;

            virtual bool IsInitialized() const override { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            Result Map(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process, ams::svc::MemoryPermission map_perm);
            Result Unmap(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process);

            u64 GetOwnerProcessId() const { return m_owner_process_id; }
            size_t GetSize() const { return m_page_group.GetNumPages() * PageSize; }
    };

}
