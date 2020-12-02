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

    class KAlpha final : public KAutoObjectWithSlabHeapAndContainer<KAlpha, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KAlpha, KAutoObject);
        private:
            /* NOTE: Official KAlpha has size 0x50, corresponding to 0x20 bytes of fields. */
            /* TODO: Add these fields, if KAlpha is ever instantiable in the NX kernel. */
        public:
            explicit KAlpha() {
                /* ... */
            }

            virtual ~KAlpha() { /* ... */ }

            /* virtual void Finalize() override; */

            virtual bool IsInitialized() const override { return false /* TODO */; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }
    };

}
