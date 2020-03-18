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
#include "os_waitable_manager_impl.hpp"
#include "os_waitable_object_list.hpp"

namespace ams::os::impl{

    WaitableHolderBase *WaitableManagerImpl::WaitAnyImpl(bool infinite, u64 timeout) {
        /* Set processing thread handle while in scope. */
        this->waiting_thread_handle = threadGetCurHandle();
        ON_SCOPE_EXIT { this->waiting_thread_handle = INVALID_HANDLE; };

        /* Prepare for processing. */
        this->signaled_holder = nullptr;
        WaitableHolderBase *result = this->LinkHoldersToObjectList();

        /* Check if we've been signaled. */
        {
            std::scoped_lock lk(this->lock);
            if (this->signaled_holder != nullptr) {
                result = this->signaled_holder;
            }
        }

        /* Process object array. */
        if (result == nullptr) {
            result = this->WaitAnyHandleImpl(infinite, timeout);
        }

        /* Unlink holders from the current object list. */
        this->UnlinkHoldersFromObjectList();

        return result;
    }

    WaitableHolderBase *WaitableManagerImpl::WaitAnyHandleImpl(bool infinite, u64 timeout) {
        Handle object_handles[MaximumHandleCount];
        WaitableHolderBase *objects[MaximumHandleCount];

        const size_t count = this->BuildHandleArray(object_handles, objects);
        const u64 end_time = infinite ? std::numeric_limits<u64>::max() : armTicksToNs(armGetSystemTick());

        while (true) {
            this->current_time = armTicksToNs(armGetSystemTick());

            u64 min_timeout = 0;
            WaitableHolderBase *min_timeout_object = this->RecalculateNextTimeout(&min_timeout, end_time);

            s32 index;
            if (count == 0 && min_timeout == 0) {
                index = WaitTimedOut;
            } else {
                index = this->WaitSynchronization(object_handles, count, min_timeout);
                AMS_ABORT_UNLESS(index != WaitInvalid);
            }

            switch (index) {
                case WaitTimedOut:
                    if (min_timeout_object) {
                        this->current_time = armTicksToNs(armGetSystemTick());
                        if (min_timeout_object->IsSignaled() == TriBool::True) {
                            std::scoped_lock lk(this->lock);
                            this->signaled_holder = min_timeout_object;
                            return this->signaled_holder;
                        }
                        continue;
                    }
                    return nullptr;
                case WaitCancelled:
                    if (this->signaled_holder) {
                        return this->signaled_holder;
                    }
                    continue;
                default: /* 0 - 0x3F, valid. */
                    {
                        std::scoped_lock lk(this->lock);
                        this->signaled_holder = objects[index];
                        return this->signaled_holder;
                    }
            }
        }
    }

    s32 WaitableManagerImpl::WaitSynchronization(Handle *handles, size_t count, u64 timeout) {
        s32 index = WaitInvalid;

        R_TRY_CATCH(svcWaitSynchronization(&index, handles, count, timeout)) {
            R_CATCH(svc::ResultTimedOut)  { return WaitTimedOut; }
            R_CATCH(svc::ResultCancelled) { return WaitCancelled; }
            /* All other results are critical errors. */
            /* svc::ResultThreadTerminating */
            /* svc::ResultInvalidHandle. */
            /* svc::ResultInvalidPointer */
            /* svc::ResultOutOfRange */
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        return index;
    }

    size_t WaitableManagerImpl::BuildHandleArray(Handle *out_handles, WaitableHolderBase **out_objects) {
        size_t count = 0;

        for (WaitableHolderBase &holder_base : this->waitable_list) {
            if (Handle handle = holder_base.GetHandle(); handle != INVALID_HANDLE) {
                AMS_ABORT_UNLESS(count < MaximumHandleCount);

                out_handles[count] = handle;
                out_objects[count] = &holder_base;
                count++;
            }
        }

        return count;
    }

    WaitableHolderBase *WaitableManagerImpl::LinkHoldersToObjectList() {
        WaitableHolderBase *signaled_holder = nullptr;

        for (WaitableHolderBase &holder_base : this->waitable_list) {
            TriBool is_signaled = holder_base.LinkToObjectList();

            if (signaled_holder == nullptr && is_signaled == TriBool::True) {
                signaled_holder = &holder_base;
            }
        }

        return signaled_holder;
    }

    void WaitableManagerImpl::UnlinkHoldersFromObjectList() {
        for (WaitableHolderBase &holder_base : this->waitable_list) {
            holder_base.UnlinkFromObjectList();
        }
    }

    WaitableHolderBase *WaitableManagerImpl::RecalculateNextTimeout(u64 *out_min_timeout, u64 end_time) {
        WaitableHolderBase *min_timeout_holder = nullptr;
        u64 min_time = end_time;

        for (WaitableHolderBase &holder_base : this->waitable_list) {
            if (const u64 cur_time = holder_base.GetWakeupTime(); cur_time < min_time) {
                min_timeout_holder = &holder_base;
                min_time = cur_time;
            }
        }

        if (min_time < this->current_time) {
            *out_min_timeout = 0;
        } else {
            *out_min_timeout = min_time - this->current_time;
        }
        return min_timeout_holder;
    }

    void WaitableManagerImpl::SignalAndWakeupThread(WaitableHolderBase *holder_base) {
        std::scoped_lock lk(this->lock);

        if (this->signaled_holder == nullptr) {
            this->signaled_holder = holder_base;
            R_ABORT_UNLESS(svcCancelSynchronization(this->waiting_thread_handle));
        }
    }

}
