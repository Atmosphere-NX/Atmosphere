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
#include <mesosphere/kern_k_readable_event.hpp>

namespace ams::kern {

    class KEvent final : public KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList, true> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KEvent, KAutoObject);
        private:
            KReadableEvent m_readable_event;
            KProcess *m_owner;
            bool m_initialized;
            bool m_readable_event_destroyed;
        public:
            constexpr explicit KEvent(util::ConstantInitializeTag)
                : KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList, true>(util::ConstantInitialize),
                  m_readable_event(util::ConstantInitialize), m_owner(), m_initialized(), m_readable_event_destroyed()
            {
                /* ... */
            }

            explicit KEvent() : m_readable_event(), m_owner(), m_initialized(), m_readable_event_destroyed() { /* ... */ }

            void Initialize();
            void Finalize();

            bool IsInitialized() const { return m_initialized; }
            uintptr_t GetPostDestroyArgument() const { return reinterpret_cast<uintptr_t>(m_owner); }

            static void PostDestroy(uintptr_t arg);

            KProcess *GetOwner() const { return m_owner; }

            KReadableEvent &GetReadableEvent() { return m_readable_event; }

            void Signal();
            void Clear();

            ALWAYS_INLINE void OnReadableEventDestroyed() { m_readable_event_destroyed = true; }
    };

}
