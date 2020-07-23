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

namespace ams::kern {

    class KTransferMemory final : public KAutoObjectWithSlabHeapAndContainer<KTransferMemory, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KTransferMemory, KAutoObject);
        private:
            TYPED_STORAGE(KPageGroup) page_group;
            KProcess *owner;
            KProcessAddress address;
            KLightLock lock;
            ams::svc::MemoryPermission owner_perm;
            bool is_initialized;
            bool is_mapped;
        public:
            explicit KTransferMemory() : owner(nullptr), address(Null<KProcessAddress>), owner_perm(ams::svc::MemoryPermission_None), is_initialized(false), is_mapped(false) {
                /* ... */
            }

            virtual ~KTransferMemory() { /* ... */ }

            Result Initialize(KProcessAddress addr, size_t size, ams::svc::MemoryPermission own_perm);
            virtual void Finalize() override;

            virtual bool IsInitialized() const override { return this->is_initialized; }
            virtual uintptr_t GetPostDestroyArgument() const override { return reinterpret_cast<uintptr_t>(this->owner); }
            static void PostDestroy(uintptr_t arg);

            Result Map(KProcessAddress address, size_t size, ams::svc::MemoryPermission map_perm);
            Result Unmap(KProcessAddress address, size_t size);

            KProcess *GetOwner() const { return this->owner; }
            KProcessAddress GetSourceAddress() { return this->address; }
            size_t GetSize() const { return this->is_initialized ? GetReference(this->page_group).GetNumPages() * PageSize : 0; }
    };

}
