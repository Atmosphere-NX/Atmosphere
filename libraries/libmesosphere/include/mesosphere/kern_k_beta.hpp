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

    class KProcess;

    class KBeta final : public KAutoObjectWithSlabHeapAndContainer<KBeta, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KBeta, KAutoObject);
        private:
            friend class KProcess;
        private:
            /* NOTE: Official KBeta has size 0x88, corresponding to 0x58 bytes of fields. */
            /* TODO: Add these fields, if KBeta is ever instantiable in the NX kernel. */
            util::IntrusiveListNode process_list_node;
        public:
            explicit KBeta()
                : process_list_node()
            {
                /* ... */
            }

            virtual ~KBeta() { /* ... */ }

            /* virtual void Finalize() override; */

            virtual bool IsInitialized() const override { return false /* TODO */; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }
    };

}
