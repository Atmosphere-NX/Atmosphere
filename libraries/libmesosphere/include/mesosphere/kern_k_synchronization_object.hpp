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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

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
            ThreadListNode *m_thread_list_head;
            ThreadListNode *m_thread_list_tail;
        protected:
            constexpr ALWAYS_INLINE explicit KSynchronizationObject(util::ConstantInitializeTag) : KAutoObjectWithList(util::ConstantInitialize), m_thread_list_head(), m_thread_list_tail() { MESOSPHERE_ASSERT_THIS(); }
            ALWAYS_INLINE explicit KSynchronizationObject() : m_thread_list_head(), m_thread_list_tail() { MESOSPHERE_ASSERT_THIS(); }

            /* NOTE: This is a virtual function which is overridden only by KDebugBase in Nintendo's kernel. */
            /* virtual void OnFinalizeSynchronizationObject() { MESOSPHERE_ASSERT_THIS(); } */

            void NotifyAvailable(Result result);
            ALWAYS_INLINE void NotifyAvailable() {
                return this->NotifyAvailable(ResultSuccess());
            }
        public:
            static Result Wait(s32 *out_index, KSynchronizationObject **objects, const s32 num_objects, s64 timeout);
        public:
            void Finalize();
            virtual bool IsSignaled() const { AMS_INFINITE_LOOP(); }

            void DumpWaiters();

            ALWAYS_INLINE void LinkNode(ThreadListNode *node) {
                /* Link the node to the list. */
                if (m_thread_list_tail == nullptr) {
                    m_thread_list_head = node;
                } else {
                    m_thread_list_tail->next = node;
                }

                m_thread_list_tail = node;
            }

            ALWAYS_INLINE void UnlinkNode(ThreadListNode *node) {
                /* Unlink the node from the list. */
                ThreadListNode *prev_ptr = reinterpret_cast<ThreadListNode *>(std::addressof(m_thread_list_head));
                ThreadListNode *prev_val = nullptr;
                ThreadListNode *prev, *tail_prev;

                do {
                    prev      = prev_ptr;
                    prev_ptr  = prev_ptr->next;
                    tail_prev = prev_val;
                    prev_val  = prev_ptr;
                } while (prev_ptr != node);

                if (m_thread_list_tail == node) {
                    m_thread_list_tail = tail_prev;
                }

                prev->next = node->next;
            }
    };

}
