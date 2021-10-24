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
#include <mesosphere/kern_k_io_region.hpp>

namespace ams::kern {

    class KIoPool final : public KAutoObjectWithSlabHeapAndContainer<KIoPool, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KIoPool, KAutoObject);
        private:
            using IoRegionList = util::IntrusiveListMemberTraits<&KIoRegion::m_pool_list_node>::ListType;
        private:
            KLightLock m_lock;
            IoRegionList m_io_region_list;
            ams::svc::IoPoolType m_pool_type;
            bool m_is_initialized;
        public:
            static bool IsValidIoPoolType(ams::svc::IoPoolType pool_type);
        public:
            explicit KIoPool() : m_is_initialized(false) {
                /* ... */
            }

            Result Initialize(ams::svc::IoPoolType pool_type);
            void Finalize();

            bool IsInitialized() const { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            Result AddIoRegion(KIoRegion *region);
            void RemoveIoRegion(KIoRegion *region);
    };

}
