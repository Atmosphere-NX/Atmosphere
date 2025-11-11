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
#include <mesosphere/kern_k_synchronization_object.hpp>

namespace ams::kern {

    class KEvent;

    class KReadableEvent : public KSynchronizationObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KReadableEvent, KSynchronizationObject);
        private:
            bool m_is_signaled;
            KEvent *m_parent;
        public:
            constexpr explicit KReadableEvent(util::ConstantInitializeTag) : KSynchronizationObject(util::ConstantInitialize), m_is_signaled(), m_parent() { MESOSPHERE_ASSERT_THIS(); }

            explicit KReadableEvent() { /* ... */ }

            void Initialize(KEvent *parent);

            constexpr KEvent *GetParent() const { return m_parent; }

            void Signal();
            Result Reset();

            void Clear() {
                MESOSPHERE_ASSERT_THIS();

                /* Try to perform a reset, ignoring whether it succeeds. */
                static_cast<void>(this->Reset());
            }

            virtual bool IsSignaled() const override;
            virtual void Destroy() override;

            /* NOTE: This is a virtual function in Nintendo's kernel. */
            /* virtual Result Reset(); */
    };

}
