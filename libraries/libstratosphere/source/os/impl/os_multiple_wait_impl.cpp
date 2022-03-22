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
#include <stratosphere.hpp>
#include "os_multiple_wait_impl.hpp"
#include "os_multiple_wait_object_list.hpp"
#include "os_tick_manager.hpp"

namespace ams::os::impl {

    Result MultiWaitImpl::WaitAnyImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, bool reply, NativeHandle reply_target) {
        /* Prepare for processing. */
        m_signaled_holder = nullptr;
        m_target_impl.SetCurrentThreadHandleForCancelWait();
        MultiWaitHolderBase *holder = this->LinkHoldersToObjectList();

        /* Check if we've been signaled. */
        {
            std::scoped_lock lk(m_cs_wait);
            if (m_signaled_holder != nullptr) {
                holder = m_signaled_holder;
            }
        }

        /* Process object array. */
        Result wait_result = ResultSuccess();
        if (holder != nullptr) {
            if (reply && reply_target != os::InvalidNativeHandle) {
                s32 index;
                wait_result = m_target_impl.TimedReplyAndReceive(std::addressof(index), nullptr, 0, 0, reply_target, TimeSpan::FromNanoSeconds(0));
                if (R_FAILED(wait_result)) {
                    holder = nullptr;
                }
            }
        } else {
            wait_result = this->WaitAnyHandleImpl(std::addressof(holder), infinite, timeout, reply, reply_target);
        }

        /* Unlink holders from the current object list. */
        this->UnlinkHoldersFromObjectList();

        m_target_impl.ClearCurrentThreadHandleForCancelWait();

        /* Set output holder. */
        *out = holder;

        return wait_result;
    }

    Result MultiWaitImpl::WaitAnyHandleImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, bool reply, NativeHandle reply_target) {
        NativeHandle object_handles[MaximumHandleCount];
        MultiWaitHolderBase *objects[MaximumHandleCount];

        const s32 count = this->BuildHandleArray(object_handles, objects, MaximumHandleCount);
        const TimeSpan end_time = infinite ? TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()) : GetCurrentTick().ToTimeSpan() + timeout;

        while (true) {
            m_current_time = GetCurrentTick().ToTimeSpan();

            TimeSpan min_timeout = 0;
            MultiWaitHolderBase *min_timeout_object = this->RecalculateNextTimeout(std::addressof(min_timeout), end_time);

            s32 index = WaitInvalid;
            Result wait_result = ResultSuccess();
            if (reply) {
                if (infinite && min_timeout_object == nullptr) {
                    wait_result = m_target_impl.ReplyAndReceive(std::addressof(index), object_handles, MaximumHandleCount, count, reply_target);
                } else {
                    wait_result = m_target_impl.TimedReplyAndReceive(std::addressof(index), object_handles, MaximumHandleCount, count, reply_target, min_timeout);
                }
            } else if (infinite && min_timeout_object == nullptr) {
                wait_result = m_target_impl.WaitAny(std::addressof(index), object_handles, MaximumHandleCount, count);
            } else {
                if (count == 0 && min_timeout == 0) {
                    index = WaitTimedOut;
                } else {
                    wait_result = m_target_impl.TimedWaitAny(std::addressof(index), object_handles, MaximumHandleCount, count, min_timeout);
                    AMS_ABORT_UNLESS(index != WaitInvalid);
                }
            }

            if (index == WaitInvalid) {
                *out = nullptr;
                return wait_result;
            }

            switch (index) {
                case WaitTimedOut:
                    if (min_timeout_object) {
                        m_current_time = GetCurrentTick().ToTimeSpan();
                        if (min_timeout_object->IsSignaled() == TriBool::True) {
                            std::scoped_lock lk(m_cs_wait);
                            m_signaled_holder = min_timeout_object;
                            *out              = min_timeout_object;
                            return wait_result;
                        }
                    } else {
                        *out = nullptr;
                        return wait_result;
                    }
                    break;
                case WaitCancelled:
                    {
                        std::scoped_lock lk(m_cs_wait);
                        if (m_signaled_holder) {
                            *out = m_signaled_holder;
                            return wait_result;
                        }
                    }
                    break;
                default: /* 0 - 0x3F, valid. */
                    {
                        AMS_ASSERT(0 <= index && index < static_cast<s32>(MaximumHandleCount));

                        std::scoped_lock lk(m_cs_wait);
                        m_signaled_holder = objects[index];
                        *out              = objects[index];
                        return wait_result;
                    }
                    break;
            }

            reply_target = os::InvalidNativeHandle;
        }
    }

    s32 MultiWaitImpl::BuildHandleArray(NativeHandle out_handles[], MultiWaitHolderBase *out_objects[], s32 num) {
        s32 count = 0;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            if (auto handle = holder_base.GetHandle(); handle != os::InvalidNativeHandle) {
                AMS_ABORT_UNLESS(count < num);

                out_handles[count] = handle;
                out_objects[count] = std::addressof(holder_base);
                count++;
            }
        }

        return count;
    }

    MultiWaitHolderBase *MultiWaitImpl::LinkHoldersToObjectList() {
        MultiWaitHolderBase *signaled_holder = nullptr;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            TriBool is_signaled = holder_base.LinkToObjectList();

            if (signaled_holder == nullptr && is_signaled == TriBool::True) {
                signaled_holder = std::addressof(holder_base);
            }
        }

        return signaled_holder;
    }

    void MultiWaitImpl::UnlinkHoldersFromObjectList() {
        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            holder_base.UnlinkFromObjectList();
        }
    }

    MultiWaitHolderBase *MultiWaitImpl::RecalculateNextTimeout(TimeSpan *out_min_timeout, TimeSpan end_time) {
        MultiWaitHolderBase *min_timeout_holder = nullptr;
        TimeSpan min_time = end_time;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            if (const TimeSpan cur_time = holder_base.GetAbsoluteWakeupTime(); cur_time < min_time) {
                min_timeout_holder = std::addressof(holder_base);
                min_time = cur_time;
            }
        }

        if (min_time < m_current_time) {
            *out_min_timeout = 0;
        } else {
            *out_min_timeout = min_time - m_current_time;
        }
        return min_timeout_holder;
    }

    void MultiWaitImpl::SignalAndWakeupThread(MultiWaitHolderBase *holder_base) {
        std::scoped_lock lk(m_cs_wait);

        if (m_signaled_holder == nullptr) {
            m_signaled_holder = holder_base;
            m_target_impl.CancelWait();
        }
    }

}
