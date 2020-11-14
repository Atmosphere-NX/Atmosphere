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
#include <mesosphere/kern_k_readable_event.hpp>
#include <mesosphere/kern_k_writable_event.hpp>

namespace ams::kern {

    class KEvent final : public KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KEvent, KAutoObject);
        private:
            KReadableEvent readable_event;
            KWritableEvent writable_event;
            KProcess *owner;
            bool initialized;
        public:
            constexpr KEvent()
                : readable_event(), writable_event(), owner(), initialized()
            {
                /* ... */
            }

            virtual ~KEvent() { /* ... */ }

            void Initialize();
            virtual void Finalize() override;

            virtual bool IsInitialized() const override { return this->initialized; }
            virtual uintptr_t GetPostDestroyArgument() const override { return reinterpret_cast<uintptr_t>(this->owner); }

            static void PostDestroy(uintptr_t arg);

            virtual KProcess *GetOwner() const override { return this->owner; }

            KReadableEvent &GetReadableEvent() { return this->readable_event; }
            KWritableEvent &GetWritableEvent() { return this->writable_event; }
    };

}
