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
#include "os_multiple_wait_holder_base.hpp"
#include "os_multiple_wait_object_list.hpp"

namespace ams::os::impl {

    class MultiWaitHolderOfMessageQueueNotEmpty : public MultiWaitHolderOfUserWaitObject {
        private:
            MessageQueueType *m_mq;
        private:
            ALWAYS_INLINE TriBool IsSignaledUnsafe() const {
                return m_mq->count > 0 ? TriBool::True : TriBool::False;
            }
        public:
            explicit MultiWaitHolderOfMessageQueueNotEmpty(MessageQueueType *mq) : m_mq(mq) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));
                return this->IsSignaledUnsafe();
            }

            virtual TriBool AddToObjectList() override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));

                GetReference(m_mq->waitlist_not_empty).PushBackToList(*this);
                return this->IsSignaledUnsafe();
            }

            virtual void RemoveFromObjectList() override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));

                GetReference(m_mq->waitlist_not_empty).EraseFromList(*this);
            }
    };

    class MultiWaitHolderOfMessageQueueNotFull : public MultiWaitHolderOfUserWaitObject {
        private:
            MessageQueueType *m_mq;
        private:
            ALWAYS_INLINE TriBool IsSignaledUnsafe() const {
                return m_mq->count < m_mq->capacity ? TriBool::True : TriBool::False;
            }
        public:
            explicit MultiWaitHolderOfMessageQueueNotFull(MessageQueueType *mq) : m_mq(mq) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));
                return this->IsSignaledUnsafe();
            }

            virtual TriBool AddToObjectList() override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));

                GetReference(m_mq->waitlist_not_full).PushBackToList(*this);
                return this->IsSignaledUnsafe();
            }

            virtual void RemoveFromObjectList() override {
                std::scoped_lock lk(GetReference(m_mq->cs_queue));

                GetReference(m_mq->waitlist_not_full).EraseFromList(*this);
            }
    };

}
