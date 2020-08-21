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
            KPageGroup page_group;
            KResourceLimit *resource_limit;
            u64 owner_process_id;
            ams::svc::MemoryPermission owner_perm;
            ams::svc::MemoryPermission remote_perm;
            bool is_initialized;
        public:
            explicit KSharedMemory()
                : page_group(std::addressof(Kernel::GetBlockInfoManager())), resource_limit(nullptr), owner_process_id(std::numeric_limits<u64>::max()),
                  owner_perm(ams::svc::MemoryPermission_None), remote_perm(ams::svc::MemoryPermission_None), is_initialized(false)
            {
                /* ... */
            }

            virtual ~KSharedMemory() { /* ... */ }

            Result Initialize(KProcess *owner, size_t size, ams::svc::MemoryPermission own_perm, ams::svc::MemoryPermission rem_perm);
            virtual void Finalize() override;

            virtual bool IsInitialized() const override { return this->is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            Result Map(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process, ams::svc::MemoryPermission map_perm);
            Result Unmap(KProcessPageTable *table, KProcessAddress address, size_t size, KProcess *process);

            u64 GetOwnerProcessId() const { return this->owner_process_id; }
            size_t GetSize() const { return this->page_group.GetNumPages() * PageSize; }
    };

}
