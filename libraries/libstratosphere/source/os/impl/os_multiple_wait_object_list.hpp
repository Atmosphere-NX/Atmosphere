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
#include "os_multiple_wait_impl.hpp"

namespace ams::os::impl {

    class MultiWaitObjectList {
        public:
            using ListType = util::IntrusiveListMemberTraits<&MultiWaitHolderBase::object_list_node>::ListType;
        private:
            ListType object_list;
        public:
            void SignalAllThreads() {
                for (MultiWaitHolderBase &holder_base : this->object_list) {
                    holder_base.GetMultiWait()->SignalAndWakeupThread(&holder_base);
                }
            }

            void BroadcastAllThreads() {
                for (MultiWaitHolderBase &holder_base : this->object_list) {
                    holder_base.GetMultiWait()->SignalAndWakeupThread(nullptr);
                }
            }

            bool IsEmpty() const {
                return this->object_list.empty();
            }

            void LinkMultiWaitHolder(MultiWaitHolderBase &holder_base) {
                this->object_list.push_back(holder_base);
            }

            void UnlinkMultiWaitHolder(MultiWaitHolderBase &holder_base) {
                this->object_list.erase(this->object_list.iterator_to(holder_base));
            }
    };

}
