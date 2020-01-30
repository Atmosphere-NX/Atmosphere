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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_linked_list.hpp>

namespace ams::kern {

    class KThread;

    class KSynchronizationObject : public KAutoObjectWithList {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSynchronizationObject, KAutoObject);
        public:
            using ThreadList = KLinkedList<KThread>;
            using iterator = ThreadList::iterator;
        private:
            ThreadList thread_list;
        protected:
            constexpr ALWAYS_INLINE explicit KSynchronizationObject() : KAutoObjectWithList(), thread_list() { MESOSPHERE_ASSERT_THIS(); }
            virtual ~KSynchronizationObject() { MESOSPHERE_ASSERT_THIS(); }

            virtual void OnFinalizeSynchronizationObject() { MESOSPHERE_ASSERT_THIS(); }

            void NotifyAvailable();
            void NotifyAbort(Result abort_reason);
        public:
            virtual void Finalize() override;
            virtual bool IsSignaled() const = 0;
            virtual void DebugWaiters();

            iterator AddWaiterThread(KThread *thread);
            iterator RemoveWaiterThread(iterator it);

            iterator begin();
            iterator end();
    };

}
