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
#include "htcs_rpc_tasks.hpp"

namespace ams::htcs::impl::rpc {

    Result SelectTask::SetArguments(Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec) {
        /* Check that we're valid. */
        R_UNLESS(this->IsValid(), htcs::ResultInvalidTask());

        /* Sanity check the spans. */
        AMS_ASSERT(0 <= read_handles.size() && read_handles.size() < static_cast<size_t>(SocketCountMax));
        AMS_ASSERT(0 <= write_handles.size() && write_handles.size() < static_cast<size_t>(SocketCountMax));
        AMS_ASSERT(0 <= exception_handles.size() && exception_handles.size() < static_cast<size_t>(SocketCountMax));

        /* Set our arguments. */
        m_read_handle_count      = static_cast<s32>(read_handles.size());
        m_write_handle_count     = static_cast<s32>(write_handles.size());
        m_exception_handle_count = static_cast<s32>(exception_handles.size());
        m_tv_sec                 = tv_sec;
        m_tv_usec                = tv_usec;

        /* Copy the handles. */
        std::memcpy(m_handles, read_handles.data(), read_handles.size_bytes());
        std::memcpy(m_handles + m_read_handle_count, write_handles.data(), write_handles.size_bytes());
        std::memcpy(m_handles + m_read_handle_count + m_write_handle_count, exception_handles.data(), exception_handles.size_bytes());

        return ResultSuccess();
    }

    void SelectTask::Complete(htcs::SocketError err, s32 read_handle_count, s32 write_handle_count, s32 exception_handle_count, const void *body, s64 body_size) {
        /* Sanity check the handle counts. */
        const auto handle_count = read_handle_count + write_handle_count + exception_handle_count;
        AMS_ASSERT(0 <= read_handle_count && read_handle_count < SocketCountMax);
        AMS_ASSERT(0 <= write_handle_count && write_handle_count < SocketCountMax);
        AMS_ASSERT(0 <= exception_handle_count && exception_handle_count < SocketCountMax);
        AMS_ASSERT(handle_count * static_cast<s64>(sizeof(s32)) == body_size);
        AMS_UNUSED(handle_count, body_size);

        /* Set our results. */
        m_err                        = err;
        m_out_read_handle_count      = read_handle_count;
        m_out_write_handle_count     = write_handle_count;
        m_out_exception_handle_count = exception_handle_count;

        /* Copy the handles. */
        std::memcpy(m_out_handles,                                          static_cast<const s32 *>(body),                                          sizeof(s32) * read_handle_count);
        std::memcpy(m_out_handles + read_handle_count,                      static_cast<const s32 *>(body) + read_handle_count,                      sizeof(s32) * write_handle_count);
        std::memcpy(m_out_handles + read_handle_count + write_handle_count, static_cast<const s32 *>(body) + read_handle_count + write_handle_count, sizeof(s32) * exception_handle_count);

        /* Complete. */
        HtcsSignalingTask::Complete();
    }

    Result SelectTask::GetResult(htcs::SocketError *out_err, bool *out_empty, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles) const {
        /* Set the output error. */
        *out_err = m_err;

        /* Set the output empty value. */
        const bool empty = m_err == HTCS_ENONE && m_out_read_handle_count == 0 && m_out_write_handle_count == 0 && m_out_exception_handle_count == 0;
        *out_empty = empty;

        /* Clear the output spans. */
        std::fill(read_handles.begin(), read_handles.end(), 0);
        std::fill(write_handles.begin(), write_handles.end(), 0);
        std::fill(exception_handles.begin(), exception_handles.end(), 0);

        /* Copy the handles. */
        if (m_err == HTCS_ENONE && !empty) {
            const s32 * const out_read_start      = m_out_handles;
            const s32 * const out_read_end        = out_read_start + m_out_read_handle_count;
            const s32 * const out_write_start     = out_read_end;
            const s32 * const out_write_end       = out_write_start + m_out_write_handle_count;
            const s32 * const out_exception_start = out_write_end;
            const s32 * const out_exception_end   = out_exception_start + m_out_exception_handle_count;
            std::copy(out_read_start,      out_read_end,      read_handles.begin());
            std::copy(out_write_start,     out_write_end,     write_handles.begin());
            std::copy(out_exception_start, out_exception_end, exception_handles.begin());
        } else {
            const s32 * const read_start      = m_handles;
            const s32 * const read_end        = read_start + m_read_handle_count;
            const s32 * const write_start     = read_end;
            const s32 * const write_end       = write_start + m_write_handle_count;
            const s32 * const exception_start = write_end;
            const s32 * const exception_end   = exception_start + m_exception_handle_count;
            std::copy(read_start,      read_end,      read_handles.begin());
            std::copy(write_start,     write_end,     write_handles.begin());
            std::copy(exception_start, exception_end, exception_handles.begin());
        }

        return ResultSuccess();
    }

    Result SelectTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), packet->params[1], packet->params[2], packet->params[3], packet->data, size - sizeof(*packet));

        return ResultSuccess();
    }

    Result SelectTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Determine the body size. */
        const auto handle_count = m_read_handle_count + m_write_handle_count + m_exception_handle_count;
        const s64 body_size     = static_cast<s64>(handle_count * sizeof(s32));
        AMS_ASSERT(sizeof(HtcsRpcPacket) + body_size <= size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Select,
            .body_size = body_size,
            .task_id   = task_id,
            .params    = {
                m_read_handle_count,
                m_write_handle_count,
                m_exception_handle_count,
                m_tv_sec,
                m_tv_usec,
            },
        };

        /* Set the packet body. */
        std::memcpy(packet->data, m_handles, body_size);

        /* Set the output size. */
        *out = sizeof(*packet) + body_size;

        return ResultSuccess();
    }

}
