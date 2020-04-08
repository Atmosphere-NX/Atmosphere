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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_waitable_manager_target_impl.os.horizon.hpp"
#else
    #error "Unknown OS for ams::os::WaitableManagerTargetImpl"
#endif

namespace ams::os::impl {

    class WaitableManagerImpl {
        public:
            static constexpr size_t MaximumHandleCount = WaitableManagerTargetImpl::MaximumHandleCount;
            static constexpr s32 WaitInvalid   = -3;
            static constexpr s32 WaitCancelled = -2;
            static constexpr s32 WaitTimedOut  = -1;
            using ListType = util::IntrusiveListMemberTraits<&WaitableHolderBase::manager_node>::ListType;
        private:
            ListType waitable_list;
            WaitableHolderBase *signaled_holder;
            TimeSpan current_time;
            InternalCriticalSection cs_wait;
            WaitableManagerTargetImpl target_impl;
        private:
            WaitableHolderBase *WaitAnyImpl(bool infinite, TimeSpan timeout);
            WaitableHolderBase *WaitAnyHandleImpl(bool infinite, TimeSpan timeout);
            s32                BuildHandleArray(Handle out_handles[], WaitableHolderBase *out_objects[], s32 num);

            WaitableHolderBase *LinkHoldersToObjectList();
            void                UnlinkHoldersFromObjectList();

            WaitableHolderBase *RecalculateNextTimeout(TimeSpan *out_min_timeout, TimeSpan end_time);
        public:
            /* Wait. */
            WaitableHolderBase *WaitAny() {
                return this->WaitAnyImpl(true, TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()));
            }

            WaitableHolderBase *TryWaitAny() {
                return this->WaitAnyImpl(false, TimeSpan(0));
            }

            WaitableHolderBase *TimedWaitAny(TimeSpan ts) {
                return this->WaitAnyImpl(false, ts);
            }

            /* List management. */
            bool IsEmpty() const {
                return this->waitable_list.empty();
            }

            void LinkWaitableHolder(WaitableHolderBase &holder_base) {
                this->waitable_list.push_back(holder_base);
            }

            void UnlinkWaitableHolder(WaitableHolderBase &holder_base) {
                this->waitable_list.erase(this->waitable_list.iterator_to(holder_base));
            }

            void UnlinkAll() {
                while (!this->IsEmpty()) {
                    this->waitable_list.front().SetManager(nullptr);
                    this->waitable_list.pop_front();
                }
            }

            void MoveAllFrom(WaitableManagerImpl &other) {
                /* Set manager for all of the other's waitables. */
                for (auto &w : other.waitable_list) {
                    w.SetManager(this);
                }
                this->waitable_list.splice(this->waitable_list.end(), other.waitable_list);
            }

            /* Other. */
            TimeSpan GetCurrentTime() const {
                return this->current_time;
            }

            void SignalAndWakeupThread(WaitableHolderBase *holder_base);
    };

}
