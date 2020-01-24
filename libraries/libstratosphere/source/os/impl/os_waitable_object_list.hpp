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
#include "os_waitable_manager_impl.hpp"

namespace ams::os::impl {

    class WaitableObjectList {
        public:
            using ListType = util::IntrusiveListMemberTraits<&WaitableHolderBase::object_list_node>::ListType;
        private:
            ListType object_list;
        public:
            void SignalAllThreads() {
                for (WaitableHolderBase &holder_base : this->object_list) {
                    holder_base.GetManager()->SignalAndWakeupThread(&holder_base);
                }
            }

            void BroadcastAllThreads() {
                for (WaitableHolderBase &holder_base : this->object_list) {
                    holder_base.GetManager()->SignalAndWakeupThread(nullptr);
                }
            }

            bool IsEmpty() const {
                return this->object_list.empty();
            }

            void LinkWaitableHolder(WaitableHolderBase &holder_base) {
                this->object_list.push_back(holder_base);
            }

            void UnlinkWaitableHolder(WaitableHolderBase &holder_base) {
                this->object_list.erase(this->object_list.iterator_to(holder_base));
            }
    };

}
