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
#include <stratosphere.hpp>
#include "sysupdater_async_impl.hpp"
#include "sysupdater_async_thread_allocator.hpp"

namespace ams::mitm::sysupdater {

    Result AsyncBase::ToAsyncResult(Result result) {
        R_TRY_CATCH(result) {
            R_CONVERT(nim::ResultHttpConnectionCanceled, ns::ResultCanceled());
            R_CONVERT(ncm::ResultInstallTaskCancelled,   ns::ResultCanceled());
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    AsyncPrepareSdCardUpdateImpl::~AsyncPrepareSdCardUpdateImpl() {
        if (this->thread_info) {
            os::WaitThread(this->thread_info->thread);
            os::DestroyThread(this->thread_info->thread);
            GetAsyncThreadAllocator()->Free(*this->thread_info);
        }
    }

    Result AsyncPrepareSdCardUpdateImpl::Run() {
        /* Get a thread info. */
        ThreadInfo info;
        R_TRY(GetAsyncThreadAllocator()->Allocate(std::addressof(info)));

        /* Set the thread info's priority. */
        info.priority = AMS_GET_SYSTEM_THREAD_PRIORITY(mitm_sysupdater, AsyncPrepareSdCardUpdateTask);

        /* Ensure that we clean up appropriately. */
        ON_SCOPE_EXIT {
            if (!this->thread_info) {
                GetAsyncThreadAllocator()->Free(info);
            }
        };

        /* Create a thread for the task. */
        R_TRY(os::CreateThread(info.thread, [](void *arg) {
            auto *_this = reinterpret_cast<AsyncPrepareSdCardUpdateImpl *>(arg);
            _this->result = _this->Execute();
            _this->event.Signal();
        }, this, info.stack, info.stack_size, info.priority));

        /* Set the thread name. */
        os::SetThreadNamePointer(info.thread, AMS_GET_SYSTEM_THREAD_NAME(mitm_sysupdater, AsyncPrepareSdCardUpdateTask));

        /* Start the thread. */
        os::StartThread(info.thread);

        /* Set our thread info. */
        this->thread_info = info;
        return ResultSuccess();
    }

    Result AsyncPrepareSdCardUpdateImpl::Execute() {
        return this->task->PrepareAndExecute();
    }

    void AsyncPrepareSdCardUpdateImpl::CancelImpl() {
        this->task->Cancel();
    }

}
