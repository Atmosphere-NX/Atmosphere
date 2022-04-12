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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_multiple_wait_target_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_multiple_wait_target_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_multiple_wait_target_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_multiple_wait_target_impl.os.macos.hpp"
#else
    #error "Unknown OS for ams::os::MultiWaitTargetImpl"
#endif

namespace ams::os::impl {

    class MultiWaitImpl {
        public:
            static constexpr size_t MaximumHandleCount = MultiWaitTargetImpl::MaximumHandleCount;
            static constexpr s32 WaitInvalid   = -3;
            static constexpr s32 WaitCancelled = -2;
            static constexpr s32 WaitTimedOut  = -1;
            using MultiWaitList = util::IntrusiveListMemberTraitsByNonConstexprOffsetOf<&MultiWaitHolderBase::m_multi_wait_node>::ListType;
        private:
            MultiWaitList m_multi_wait_list;
            MultiWaitHolderBase *m_signaled_holder;
            TimeSpan m_current_time;
            InternalCriticalSection m_cs_wait;
            MultiWaitTargetImpl m_target_impl;
        private:
            template<bool AllowReply>
            Result WaitAnyImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, NativeHandle reply_target);

            template<bool AllowReply>
            Result InternalWaitAnyImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, NativeHandle reply_target);

            s32 ConstructObjectsArray(NativeHandle out_handles[], MultiWaitHolderBase *out_objects[], s32 num);

            MultiWaitHolderBase *AddToEachObjectListAndCheckObjectState();
            void                 RemoveFromEachObjectList();

            MultiWaitHolderBase *RecalcMultiWaitTimeout(TimeSpan *out_min_timeout, TimeSpan end_time);

            MultiWaitHolderBase *WaitAnyImpl(bool infinite, TimeSpan timeout);
        public:
            /* Wait. */
            MultiWaitHolderBase *WaitAny() {
                return this->WaitAnyImpl(true, TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()));
            }

            MultiWaitHolderBase *TryWaitAny() {
                return this->WaitAnyImpl(false, TimeSpan(0));
            }

            MultiWaitHolderBase *TimedWaitAny(TimeSpan ts) {
                return this->WaitAnyImpl(false, ts);
            }

            Result ReplyAndReceive(MultiWaitHolderBase **out, NativeHandle reply_target);

            /* List management. */
            bool IsListEmpty() const {
                return m_multi_wait_list.empty();
            }

            bool IsListNotEmpty() const {
                return !m_multi_wait_list.empty();
            }

            void PushBackToList(MultiWaitHolderBase &holder_base) {
                m_multi_wait_list.push_back(holder_base);
            }

            void EraseFromList(MultiWaitHolderBase &holder_base) {
                m_multi_wait_list.erase(m_multi_wait_list.iterator_to(holder_base));
            }

            void EraseAllFromList() {
                while (!m_multi_wait_list.empty()) {
                    m_multi_wait_list.front().SetMultiWait(nullptr);
                    m_multi_wait_list.pop_front();
                }
            }

            void MoveAllFromOther(MultiWaitImpl &other) {
                /* Set ourselves as multi wait for all of the other's holders. */
                for (auto &w : other.m_multi_wait_list) {
                    w.SetMultiWait(this);
                }

                m_multi_wait_list.splice(m_multi_wait_list.end(), other.m_multi_wait_list);
            }

            /* Other. */
            TimeSpan GetCurrTime() const {
                return m_current_time;
            }

            void NotifyAndWakeupThread(MultiWaitHolderBase *holder_base);
    };

}
