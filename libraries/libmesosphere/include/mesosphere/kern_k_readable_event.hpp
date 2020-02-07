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

    class KEvent;

    class KReadableEvent : public KSynchronizationObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KReadableEvent, KSynchronizationObject);
        private:
            bool is_signaled;
            KEvent *parent_event;
        public:
            constexpr explicit KReadableEvent() : KSynchronizationObject(), is_signaled(), parent_event() { MESOSPHERE_ASSERT_THIS(); }
            virtual ~KReadableEvent() { MESOSPHERE_ASSERT_THIS(); }

            constexpr void Initialize(KEvent *parent) {
                MESOSPHERE_ASSERT_THIS();
                this->is_signaled  = false;
                this->parent_event = parent;
            }

            constexpr KEvent *GetParent() const { return this->parent_event; }

            virtual bool IsSignaled() const override;
            virtual void Destroy() override;

            virtual Result Signal();
            virtual Result Clear();
            virtual Result Reset();
    };

}
