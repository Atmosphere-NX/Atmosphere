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
#include "os_waitable_holder_base.hpp"
#include "os_waitable_object_list.hpp"

namespace ams::os::impl {

    template<MessageQueueWaitType WaitType>
    class WaitableHolderOfMessageQueue : public WaitableHolderOfUserObject {
        static_assert(WaitType == MessageQueueWaitType::ForNotEmpty || WaitType == MessageQueueWaitType::ForNotFull);
        private:
            MessageQueueType *mq;
        private:
            constexpr inline TriBool IsSignaledImpl() const {
                if constexpr (WaitType == MessageQueueWaitType::ForNotEmpty) {
                    /* ForNotEmpty. */
                    return this->mq->count > 0 ? TriBool::True : TriBool::False;
                } else if constexpr (WaitType == MessageQueueWaitType::ForNotFull) {
                    /* ForNotFull */
                    return this->mq->count < this->mq->capacity ? TriBool::True : TriBool::False;
                } else {
                    static_assert(WaitType != WaitType);
                }
            }

            constexpr inline WaitableObjectList &GetObjectList() const {
                if constexpr (WaitType == MessageQueueWaitType::ForNotEmpty) {
                    return GetReference(this->mq->waitlist_not_empty);
                } else if constexpr (WaitType == MessageQueueWaitType::ForNotFull) {
                    return GetReference(this->mq->waitlist_not_full);
                } else {
                    static_assert(WaitType != WaitType);
                }
            }
        public:
            explicit WaitableHolderOfMessageQueue(MessageQueueType *mq) : mq(mq) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(GetReference(this->mq->cs_queue));
                return this->IsSignaledImpl();
            }

            virtual TriBool LinkToObjectList() override {
                std::scoped_lock lk(GetReference(this->mq->cs_queue));

                this->GetObjectList().LinkWaitableHolder(*this);
                return this->IsSignaledImpl();
            }

            virtual void UnlinkFromObjectList() override {
                std::scoped_lock lk(GetReference(this->mq->cs_queue));

                this->GetObjectList().UnlinkWaitableHolder(*this);
            }
    };

    using WaitableHolderOfMessageQueueForNotEmpty = WaitableHolderOfMessageQueue<MessageQueueWaitType::ForNotEmpty>;
    using WaitableHolderOfMessageQueueForNotFull  = WaitableHolderOfMessageQueue<MessageQueueWaitType::ForNotFull>;

}
