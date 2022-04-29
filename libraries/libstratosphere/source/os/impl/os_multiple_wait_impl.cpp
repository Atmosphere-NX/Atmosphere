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

    template<bool AllowReply>
    Result MultiWaitImpl::WaitAnyImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, NativeHandle reply_target) {
        /* Prepare for processing. */
        m_signaled_holder = nullptr;
        m_target_impl.SetCurrentThreadHandleForCancelWait();

        /* Add each holder to the object list, and try to find one that's signaled. */
        MultiWaitHolderBase *signaled_holder = this->AddToEachObjectListAndCheckObjectState();

        /* When we're done, cleanup and set output. */
        ON_SCOPE_EXIT {
            /* Remove each holder from the current object list. */
            this->RemoveFromEachObjectList();

            /* Clear cancel wait. */
            m_target_impl.ClearCurrentThreadHandleForCancelWait();

            /* Set output holder. */
            *out = signaled_holder;
        };

        /* Check if we've been signaled. */
        {
            std::scoped_lock lk(m_cs_wait);
            if (m_signaled_holder != nullptr) {
                signaled_holder = m_signaled_holder;
            }
        }

        /* Process object array. */
        if (signaled_holder != nullptr) {
            /* If we have a signaled holder and we're allowed to reply, try to do so. */
            if constexpr (AllowReply) {
                /* Try to reply to the reply target. */
                if (reply_target != os::InvalidNativeHandle) {
                    ON_RESULT_FAILURE { signaled_holder = nullptr; };

                    s32 index;
                    R_TRY(m_target_impl.TimedReplyAndReceive(std::addressof(index), nullptr, 0, 0, reply_target, TimeSpan::FromNanoSeconds(0)));
                }
            }
        } else {
            /* If there's no signaled holder, wait for one to be signaled. */
            R_TRY(this->InternalWaitAnyImpl<AllowReply>(std::addressof(signaled_holder), infinite, timeout, reply_target));
        }

        R_SUCCEED();
    }

    template<bool AllowReply>
    Result MultiWaitImpl::InternalWaitAnyImpl(MultiWaitHolderBase **out, bool infinite, TimeSpan timeout, NativeHandle reply_target) {
        /* Build the objects array. */
        NativeHandle object_handles[MaximumHandleCount];
        MultiWaitHolderBase *objects[MaximumHandleCount];

        const s32 count = this->ConstructObjectsArray(object_handles, objects, MaximumHandleCount);

        /* Determine the appropriate end time for our wait. */
        const TimeSpan end_time = infinite ? TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()) : os::impl::GetCurrentTick().ToTimeSpan() + timeout;

        /* Loop, waiting until we're done. */
        while (true) {
            /* Update the current time for our loop. */
            m_current_time = os::impl::GetCurrentTick().ToTimeSpan();

            /* Determine which object has the minimum wakeup time. */
            TimeSpan min_timeout = 0;
            MultiWaitHolderBase *min_timeout_object = this->RecalcMultiWaitTimeout(std::addressof(min_timeout), end_time);

            /* Perform the wait using native apis. */
            s32 index         = WaitInvalid;
            Result wait_result = ResultSuccess();
            if (infinite && min_timeout_object == nullptr) {
                /* If we're performing an infinite wait, just do the appropriate wait or reply/receive. */
                if constexpr (AllowReply) {
                    wait_result = m_target_impl.ReplyAndReceive(std::addressof(index), object_handles, MaximumHandleCount, count, reply_target);
                } else {
                    wait_result = m_target_impl.WaitAny(std::addressof(index), object_handles, MaximumHandleCount, count);
                }
            } else {
                /* We need to do our wait with a timeout. */
                if constexpr (AllowReply) {
                    wait_result = m_target_impl.TimedReplyAndReceive(std::addressof(index), object_handles, MaximumHandleCount, count, reply_target, min_timeout);
                } else {
                    if (count != 0 || min_timeout != 0) {
                        wait_result = m_target_impl.TimedWaitAny(std::addressof(index), object_handles, MaximumHandleCount, count, min_timeout);
                    } else {
                        index = WaitTimedOut;
                    }
                }
            }

            /* Process the result of our wait. */
            switch (index) {
                case WaitInvalid:
                    /* If an invalid wait was performed, just return no signaled holder. */
                    {
                        *out = nullptr;
                        R_RETURN(wait_result);
                    }
                    break;
                case WaitCancelled:
                    /* If the wait was canceled, it might be because a non-native waitable was signaled. Check and return it, if this is the case. */
                    {
                        std::scoped_lock lk(m_cs_wait);

                        if (m_signaled_holder) {
                            *out = m_signaled_holder;
                            R_RETURN(wait_result);
                        }
                    }
                    break;
                case WaitTimedOut:
                    /* If we timed out, this might have been because a timer is now signaled. */
                    if (min_timeout_object != nullptr) {
                        /* Update our current time. */
                        m_current_time = GetCurrentTick().ToTimeSpan();

                        /* Check if the minimum timeout object is now signaled. */
                        if (min_timeout_object->IsSignaled() == TriBool::True) {
                            std::scoped_lock lk(m_cs_wait);

                            /* Set our signaled holder (and the output) as the newly signaled minimum timeout object. */
                            m_signaled_holder = min_timeout_object;
                            *out              = min_timeout_object;
                            R_RETURN(wait_result);
                        }
                    } else {
                        /* If we have no minimum timeout object but we timed out, just return no signaled holder. */
                        *out = nullptr;
                        R_RETURN(wait_result);
                    }
                    break;
                default: /* 0 - 0x3F, valid. */
                    {
                        /* Sanity check that the returned index is within the range of our objects array. */
                        AMS_ASSERT(0 <= index && index < count);

                        std::scoped_lock lk(m_cs_wait);

                        /* Set our signaled holder (and the output) as the newly signaled object. */
                        m_signaled_holder = objects[index];
                        *out              = objects[index];
                        R_RETURN(wait_result);
                    }
                    break;
            }

            /* We're going to be looping again; prevent ourselves from replying to the same object twice. */
            if constexpr (AllowReply) {
                reply_target = os::InvalidNativeHandle;
            }
        }
    }

    MultiWaitHolderBase *MultiWaitImpl::WaitAnyImpl(bool infinite, TimeSpan timeout) {
        MultiWaitHolderBase *holder = nullptr;

        const Result wait_result = this->WaitAnyImpl<false>(std::addressof(holder), infinite, timeout, os::InvalidNativeHandle);
        R_ASSERT(wait_result);
        AMS_UNUSED(wait_result);

        return holder;
    }

    Result MultiWaitImpl::ReplyAndReceive(MultiWaitHolderBase **out, NativeHandle reply_target) {
        R_RETURN(this->WaitAnyImpl<true>(out, true, TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max()), reply_target));
    }

    s32 MultiWaitImpl::ConstructObjectsArray(NativeHandle out_handles[], MultiWaitHolderBase *out_objects[], s32 num) {
        /* Add all objects with a native handle to the output array. */
        s32 count = 0;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            os::NativeHandle handle = os::InvalidNativeHandle;
            if (holder_base.GetNativeHandle(std::addressof(handle))) {
                AMS_ABORT_UNLESS(count < num);

                out_handles[count] = handle;
                out_objects[count] = std::addressof(holder_base);

                ++count;
            }
        }

        return count;
    }

    MultiWaitHolderBase *MultiWaitImpl::AddToEachObjectListAndCheckObjectState() {
        /* Add each holder to the current object list, checking for the first signaled object. */
        MultiWaitHolderBase *signaled_holder = nullptr;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            if (const TriBool is_signaled = holder_base.AddToObjectList(); signaled_holder == nullptr && is_signaled == TriBool::True) {
                signaled_holder = std::addressof(holder_base);
            }
        }

        return signaled_holder;
    }

    void MultiWaitImpl::RemoveFromEachObjectList() {
        /* Remove each holder from the current object list. */
        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            holder_base.RemoveFromObjectList();
        }
    }

    MultiWaitHolderBase *MultiWaitImpl::RecalcMultiWaitTimeout(TimeSpan *out_min_timeout, TimeSpan end_time) {
        /* Find the holder with the minimum end time. */
        MultiWaitHolderBase *min_timeout_holder = nullptr;
        TimeSpan min_time = end_time;

        for (MultiWaitHolderBase &holder_base : m_multi_wait_list) {
            if (const TimeSpan cur_time = holder_base.GetAbsoluteTimeToWakeup(); cur_time < min_time) {
                min_timeout_holder = std::addressof(holder_base);
                min_time           = cur_time;
            }
        }

        /* If the minimum time is under the current time, we can't wait; otherwise, get the time to the minimum end time. */
        if (min_time < m_current_time) {
            *out_min_timeout = 0;
        } else {
            *out_min_timeout = min_time - m_current_time;
        }

        return min_timeout_holder;
    }

    void MultiWaitImpl::NotifyAndWakeupThread(MultiWaitHolderBase *holder_base) {
        std::scoped_lock lk(m_cs_wait);

        /* If we don't have a signaled holder, set our signaled holder. */
        if (m_signaled_holder == nullptr) {
            m_signaled_holder = holder_base;

            /* Cancel any ongoing waits. */
            m_target_impl.CancelWait();
        }
    }

}
