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
#include <mesosphere/kern_k_synchronization_object.hpp>

namespace ams::kern {

    class KSession;

    class KClientSession final : public KAutoObjectWithSlabHeapAndContainer<KClientSession, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KClientSession, KAutoObject);
        private:
            KSession *parent;
        public:
            constexpr KClientSession() : parent() { /* ... */ }
            virtual ~KClientSession() { /* ... */ }

            void Initialize(KSession *parent) {
                /* Set member variables. */
                this->parent = parent;
            }

            virtual void Destroy() override;
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            constexpr KSession *GetParent() const { return this->parent; }

            Result SendSyncRequest(uintptr_t address, size_t size);
            Result SendAsyncRequest(KWritableEvent *event, uintptr_t address, size_t size);

            void OnServerClosed();
    };

}
