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

    class KCodeMemory final : public KAutoObjectWithSlabHeapAndContainer<KCodeMemory, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KCodeMemory, KAutoObject);
        private:
            TYPED_STORAGE(KPageGroup) m_page_group;
            KProcess *m_owner;
            KProcessAddress m_address;
            KLightLock m_lock;
            bool m_is_initialized;
            bool m_is_owner_mapped;
            bool m_is_mapped;
        public:
            explicit KCodeMemory() : m_owner(nullptr), m_address(Null<KProcessAddress>), m_is_initialized(false), m_is_owner_mapped(false), m_is_mapped(false) {
                /* ... */
            }

            virtual ~KCodeMemory() { /* ... */ }

            Result Initialize(KProcessAddress address, size_t size);
            virtual void Finalize() override;

            Result Map(KProcessAddress address, size_t size);
            Result Unmap(KProcessAddress address, size_t size);
            Result MapToOwner(KProcessAddress address, size_t size, ams::svc::MemoryPermission perm);
            Result UnmapFromOwner(KProcessAddress address, size_t size);

            virtual bool IsInitialized() const override { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            KProcess *GetOwner() const { return m_owner; }
            KProcessAddress GetSourceAddress() { return m_address; }
            size_t GetSize() const { return m_is_initialized ? GetReference(m_page_group).GetNumPages() * PageSize : 0; }
    };

}
