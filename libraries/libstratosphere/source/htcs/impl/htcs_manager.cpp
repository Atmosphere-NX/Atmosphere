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
#include "htcs_manager.hpp"
#include "htcs_manager_impl.hpp"
#include "htcs_util.hpp"

namespace ams::htcs::impl {

    HtcsManager::HtcsManager(mem::StandardAllocator *allocator, htclow::HtclowManager *htclow_manager) : m_allocator(allocator), m_impl(static_cast<HtcsManagerImpl *>(allocator->Allocate(sizeof(HtcsManagerImpl), alignof(HtcsManagerImpl)))) {
        std::construct_at(m_impl, m_allocator, htclow_manager);
    }

    HtcsManager::~HtcsManager() {
        std::destroy_at(m_impl);
        m_allocator->Free(m_impl);
    }

    os::EventType *HtcsManager::GetServiceAvailabilityEvent() {
        return m_impl->GetServiceAvailabilityEvent();
    }

    bool HtcsManager::IsServiceAvailable() {
        return m_impl->IsServiceAvailable();
    }

    void HtcsManager::Socket(s32 *out_err, s32 *out_desc, bool enable_disconnection_emulation) {
        /* Invoke our implementation. */
        s32 err, desc;
        const Result result = m_impl->CreateSocket(std::addressof(err), std::addressof(desc), enable_disconnection_emulation);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_desc = desc;
            } else {
                *out_desc = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_desc = -1;
        }
    }

    void HtcsManager::Close(s32 *out_err, s32 *out_res, s32 desc) {
        /* Invoke our implementation. */
        const Result result = m_impl->DestroySocket(desc);

        /* Set output. */
        *out_err  = ConvertResultToErrorCode(result);
        if (R_SUCCEEDED(result)) {
            *out_res = 0;
        } else {
            *out_res = -1;
        }
    }

    void HtcsManager::Connect(s32 *out_err, s32 *out_res, const SockAddrHtcs &address, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        const Result result = m_impl->Connect(std::addressof(err), desc, address);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_res = 0;
            } else {
                *out_res = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_res = -1;
        }
    }

    void HtcsManager::Bind(s32 *out_err, s32 *out_res, const SockAddrHtcs &address, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        const Result result = m_impl->Bind(std::addressof(err), desc, address);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_res = 0;
            } else {
                *out_res = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_res = -1;
        }
    }

    void HtcsManager::Listen(s32 *out_err, s32 *out_res, s32 backlog_count, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        const Result result = m_impl->Listen(std::addressof(err), desc, backlog_count);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_res = 0;
            } else {
                *out_res = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_res = -1;
        }
    }

    void HtcsManager::Recv(s32 *out_err, s64 *out_size, char *buffer, size_t size, s32 flags, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 recv_size;
        const Result result = m_impl->Receive(std::addressof(err), std::addressof(recv_size), buffer, size, desc, flags);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = recv_size;
            } else {
                *out_size = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_size = -1;
        }
    }

    void HtcsManager::Send(s32 *out_err, s64 *out_size, const char *buffer, size_t size, s32 flags, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 send_size;
        const Result result = m_impl->Send(std::addressof(err), std::addressof(send_size), buffer, size, desc, flags);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = send_size;
            } else {
                *out_size = -1;
            }
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_size = -1;
        }
    }

    void HtcsManager::Shutdown(s32 *out_err, s32 *out_res, s32 how, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        const Result result = m_impl->Shutdown(std::addressof(err), desc, how);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_res = 0;
            } else {
                *out_res = -1;
            }
        } else {
            if (htcs::ResultInvalidHandle::Includes(result)) {
                *out_err = HTCS_ENOTCONN;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
            *out_res = -1;
        }
    }

    void HtcsManager::Fcntl(s32 *out_err, s32 *out_res, s32 command, s32 value, s32 desc) {
        /* Invoke our implementation. */
        s32 err, res;
        const Result result = m_impl->Fcntl(std::addressof(err), std::addressof(res), desc, command, value);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            *out_res = res;
        } else {
            *out_err  = ConvertResultToErrorCode(result);
            *out_res = -1;
        }
    }

    Result HtcsManager::AcceptStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc) {
        return m_impl->AcceptStart(out_task_id, out_handle, desc);
    }

    void HtcsManager::AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        const Result result = m_impl->AcceptResults(std::addressof(err), out_desc, out_address, task_id, desc);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
        } else {
            if (htc::ResultCancelled::Includes(result)) {
                *out_err = HTCS_ENETDOWN;
            } else if (htc::ResultTaskQueueNotAvailable::Includes(result)) {
                *out_err = HTCS_EINTR;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
        }
    }

    Result HtcsManager::RecvStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags) {
        return m_impl->RecvStart(out_task_id, out_handle, size, desc, flags);
    }

    void HtcsManager::RecvResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 size;
        const Result result = m_impl->RecvResults(std::addressof(err), std::addressof(size), buffer, buffer_size, task_id, desc);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = size;
            } else {
                *out_size = -1;
            }
        } else {
            if (htc::ResultTaskQueueNotAvailable::Includes(result)) {
                *out_err = HTCS_EINTR;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
            *out_size = -1;
        }
    }

    Result HtcsManager::SendStart(u32 *out_task_id, os::NativeHandle *out_handle, const char *buffer, s64 size, s32 desc, s32 flags) {
        return m_impl->SendStart(out_task_id, out_handle, buffer, size, desc, flags);
    }

    Result HtcsManager::SendLargeStart(u32 *out_task_id, os::NativeHandle *out_handle, const char **buffers, const s64 *sizes, s32 count, s32 desc, s32 flags) {
        return m_impl->SendLargeStart(out_task_id, out_handle, buffers, sizes, count, desc, flags);
    }

    void HtcsManager::SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 size;
        const Result result = m_impl->SendResults(std::addressof(err), std::addressof(size), task_id, desc);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = size;
            } else {
                *out_size = -1;
            }
        } else {
            if (htc::ResultTaskQueueNotAvailable::Includes(result)) {
                *out_err = HTCS_EINTR;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
            *out_size = -1;
        }
    }

    Result HtcsManager::StartSend(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags) {
        return m_impl->StartSend(out_task_id, out_handle, desc, size, flags);
    }

    Result HtcsManager::ContinueSend(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s64 size;
        R_TRY_CATCH(m_impl->ContinueSend(std::addressof(size), buffer, buffer_size, task_id, desc)) {
            R_CONVERT(htclow::ResultInvalidChannelState, tma::ResultUnknown())
            R_CONVERT(htc::ResultTaskCancelled,          tma::ResultUnknown())
        } R_END_TRY_CATCH;

        /* Set output. */
        *out_size = size;
        return ResultSuccess();
    }

    void HtcsManager::EndSend(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 size;
        const Result result = m_impl->EndSend(std::addressof(err), std::addressof(size), task_id, desc);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = size;
            } else {
                *out_size = -1;
            }
        } else {
            if (htc::ResultTaskQueueNotAvailable::Includes(result)) {
                *out_err = HTCS_EINTR;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
            *out_size = -1;
        }
    }

    Result HtcsManager::StartRecv(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags) {
        return m_impl->StartRecv(out_task_id, out_handle, size, desc, flags);
    }

    void HtcsManager::EndRecv(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Invoke our implementation. */
        s32 err;
        s64 size;
        const Result result = m_impl->EndRecv(std::addressof(err), std::addressof(size), buffer, buffer_size, task_id, desc);

        /* Set output. */
        if (R_SUCCEEDED(result)) {
            *out_err = err;
            if (err == 0) {
                *out_size = size;
            } else {
                *out_size = -1;
            }
        } else {
            if (htc::ResultCancelled::Includes(result) || htc::ResultTaskQueueNotAvailable::Includes(result)) {
                *out_err = 0;
            } else {
                *out_err = ConvertResultToErrorCode(result);
            }
            *out_size = -1;
        }
    }

    Result HtcsManager::StartSelect(u32 *out_task_id, os::NativeHandle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec) {
        /* Invoke our implementation. */
        R_TRY_CATCH(m_impl->StartSelect(out_task_id, out_handle, read_handles, write_handles, exception_handles, tv_sec, tv_usec)) {
            R_CONVERT(htc::ResultTaskCancelled, tma::ResultUnknown())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result HtcsManager::EndSelect(s32 *out_err, s32 *out_count, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id) {
        /* Invoke our implementation. */
        s32 err;
        bool empty;
        const Result result = m_impl->EndSelect(std::addressof(err), std::addressof(empty), read_handles, write_handles, exception_handles, task_id);

        /* Set output. */
        if (R_SUCCEEDED(result) && !empty) {
            *out_err = err;
            if (err == 0) {
                const auto num_read      = std::count_if(read_handles.begin(),      read_handles.end(),      [](int handle) { return handle != 0; });
                const auto num_write     = std::count_if(write_handles.begin(),     write_handles.end(),     [](int handle) { return handle != 0; });
                const auto num_exception = std::count_if(exception_handles.begin(), exception_handles.end(), [](int handle) { return handle != 0; });
                *out_count = num_read + num_write + num_exception;
            } else {
                *out_count = -1;
            }
        } else {
            if (R_SUCCEEDED(result)) {
                *out_err   = 0;
                *out_count = 0;
            } else {
                *out_err   = ConvertResultToErrorCode(err);
                *out_count = -1;
            }
        }

        return ResultSuccess();
    }

}
