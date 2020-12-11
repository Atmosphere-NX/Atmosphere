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
            struct ThreadListNode {
                ThreadListNode *next;
                KThread *thread;
            };
        private:
            ThreadListNode *thread_list_head;
            ThreadListNode *thread_list_tail;
        protected:
            constexpr ALWAYS_INLINE explicit KSynchronizationObject() : KAutoObjectWithList(), thread_list_head(), thread_list_tail() { MESOSPHERE_ASSERT_THIS(); }
            virtual ~KSynchronizationObject() { MESOSPHERE_ASSERT_THIS(); }

            virtual void OnFinalizeSynchronizationObject() { MESOSPHERE_ASSERT_THIS(); }

            void NotifyAvailable(Result result);
            void NotifyAvailable() {
                return this->NotifyAvailable(ResultSuccess());
            }
        public:
            static Result Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout);
        public:
            virtual void Finalize() override;
            virtual bool IsSignaled() const = 0;
            virtual void DumpWaiters();
    };

}
