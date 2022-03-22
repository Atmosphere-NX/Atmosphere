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
#include "htc_htcmisc_rpc_tasks.hpp"

namespace ams::htc::server::rpc {

    Result GetEnvironmentVariableTask::SetArguments(const char *args, size_t size) {
        /* Copy to our name. */
        const size_t copied = util::Strlcpy(m_name, args, sizeof(m_name));
        m_name_size = copied;

        /* Require that the size be correct. */

        R_UNLESS(size == copied || size == copied + 1, htc::ResultUnknown());

        return ResultSuccess();
    }

    void GetEnvironmentVariableTask::Complete(HtcmiscResult result, const char *data, size_t size) {
        /* Sanity check input. */
        if (size < sizeof(m_value)) {
            /* Convert the result. */
            switch (result) {
                case HtcmiscResult::Success:
                    /* Copy to our value. */
                    std::memcpy(m_value, data, size);
                    m_value[size] = '\x00';
                    m_value_size = size + 1;

                    m_result = ResultSuccess();
                    break;
                case HtcmiscResult::UnknownError:
                    m_result = htc::ResultUnknown();
                    break;
                case HtcmiscResult::UnsupportedVersion:
                    m_result = htc::ResultConnectionFailure();
                    break;
                case HtcmiscResult::InvalidRequest:
                    m_result = htc::ResultNotFound();
                    break;
            }
        } else {
            m_result = htc::ResultUnknown();
        }

        /* Complete the task. */
        Task::Complete();
    }

    Result GetEnvironmentVariableTask::GetResult(size_t *out, char *dst, size_t size) const {
        /* Check our task state. */
        AMS_ASSERT(this->GetTaskState() == RpcTaskState::Completed);

        /* Check that we succeeded. */
        R_TRY(m_result);

        /* Check that we can convert successfully. */
        R_UNLESS(util::IsIntValueRepresentable<int>(size), htc::ResultUnknown());

        /* Copy out. */
        const auto copied = util::Strlcpy(dst, m_value, size);
        R_UNLESS(copied < static_cast<int>(size), htc::ResultNotEnoughBuffer());

        /* Set the output size. */
        *out = m_value_size;

        return ResultSuccess();
    }

    Result GetEnvironmentVariableTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size >= sizeof(HtcmiscRpcPacket));
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcmiscRpcPacket *>(data);
        *packet = {
            .protocol  = HtcmiscProtocol,
            .version   = HtcmiscMaxVersion,
            .category  = HtcmiscPacketCategory::Request,
            .type      = HtcmiscPacketType::GetEnvironmentVariable,
            .body_size = this->GetNameSize(),
            .task_id   = task_id,
            .params    = {
                /* ... */
            },
        };

        /* Set the packet body. */
        std::memcpy(packet->data, this->GetName(), this->GetNameSize());

        /* Set the output size. */
        *out = sizeof(*packet) + this->GetNameSize();

        return ResultSuccess();
    }

    Result GetEnvironmentVariableTask::ProcessResponse(const char *data, size_t size) {
        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcmiscRpcPacket *>(data);

        /* Process the packet. */
        this->Complete(static_cast<HtcmiscResult>(packet->params[0]), data + sizeof(*packet), size - sizeof(*packet));

        /* Complete the task. */
        Task::Complete();

        return ResultSuccess();
    }

    Result GetEnvironmentVariableLengthTask::SetArguments(const char *args, size_t size) {
        /* Copy to our name. */
        const size_t copied = util::Strlcpy(m_name, args, sizeof(m_name));
        m_name_size = copied;

        /* Require that the size be correct. */

        R_UNLESS(size == copied || size == copied + 1, htc::ResultUnknown());

        return ResultSuccess();
    }

    void GetEnvironmentVariableLengthTask::Complete(HtcmiscResult result, const char *data, size_t size) {
        /* Sanity check input. */
        if (size == sizeof(s64)) {
            /* Convert the result. */
            switch (result) {
                case HtcmiscResult::Success:
                    /* Copy to our value. */
                    s64 tmp;
                    std::memcpy(std::addressof(tmp), data, sizeof(tmp));
                    if (util::IsIntValueRepresentable<size_t>(tmp)) {
                        m_value_size = static_cast<size_t>(tmp);
                    }

                    m_result = ResultSuccess();
                    break;
                case HtcmiscResult::UnknownError:
                    m_result = htc::ResultUnknown();
                    break;
                case HtcmiscResult::UnsupportedVersion:
                    m_result = htc::ResultConnectionFailure();
                    break;
                case HtcmiscResult::InvalidRequest:
                    m_result = htc::ResultNotFound();
                    break;
            }
        } else {
            m_result = htc::ResultUnknown();
        }

        /* Complete the task. */
        Task::Complete();
    }

    Result GetEnvironmentVariableLengthTask::GetResult(size_t *out) const {
        /* Check our task state. */
        AMS_ASSERT(this->GetTaskState() == RpcTaskState::Completed);

        /* Check that we succeeded. */
        R_TRY(m_result);

        /* Set the output size. */
        *out = m_value_size;

        return ResultSuccess();
    }

    Result GetEnvironmentVariableLengthTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size >= sizeof(HtcmiscRpcPacket));
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcmiscRpcPacket *>(data);
        *packet = {
            .protocol  = HtcmiscProtocol,
            .version   = HtcmiscMaxVersion,
            .category  = HtcmiscPacketCategory::Request,
            .type      = HtcmiscPacketType::GetEnvironmentVariableLength,
            .body_size = this->GetNameSize(),
            .task_id   = task_id,
            .params    = {
                /* ... */
            },
        };

        /* Set the packet body. */
        std::memcpy(packet->data, this->GetName(), this->GetNameSize());

        /* Set the output size. */
        *out = sizeof(*packet) + this->GetNameSize();

        return ResultSuccess();
    }

    Result GetEnvironmentVariableLengthTask::ProcessResponse(const char *data, size_t size) {
        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcmiscRpcPacket *>(data);

        /* Process the packet. */
        this->Complete(static_cast<HtcmiscResult>(packet->params[0]), data + sizeof(*packet), size - sizeof(*packet));

        /* Complete the task. */
        Task::Complete();

        return ResultSuccess();
    }

    Result RunOnHostTask::SetArguments(const char *args, size_t size) {
        /* Verify command fits in our buffer. */
        R_UNLESS(size < sizeof(m_command), htc::ResultNotEnoughBuffer());

        /* Set our command. */
        std::memcpy(m_command, args, size);
        m_command_size = size;

        return ResultSuccess();
    }

    void RunOnHostTask::Complete(int host_result) {
        /* Set our host result. */
        m_host_result = host_result;

        /* Signal. */
        m_system_event.Signal();

        /* Complete the task. */
        Task::Complete();
    }

    Result RunOnHostTask::GetResult(int *out) const {
        *out = m_host_result;
        return ResultSuccess();
    }

    void RunOnHostTask::Cancel(RpcTaskCancelReason reason) {
        /* Cancel the task. */
        Task::Cancel(reason);

        /* Signal our event. */
        m_system_event.Signal();
    }

    Result RunOnHostTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size >= sizeof(HtcmiscRpcPacket));
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcmiscRpcPacket *>(data);
        *packet = {
            .protocol  = HtcmiscProtocol,
            .version   = HtcmiscMaxVersion,
            .category  = HtcmiscPacketCategory::Request,
            .type      = HtcmiscPacketType::RunOnHost,
            .body_size = this->GetCommandSize(),
            .task_id   = task_id,
            .params    = {
                /* ... */
            },
        };

        /* Set the packet body. */
        std::memcpy(packet->data, this->GetCommand(), this->GetCommandSize());

        /* Set the output size. */
        *out = sizeof(*packet) + this->GetCommandSize();

        return ResultSuccess();
    }

    Result RunOnHostTask::ProcessResponse(const char *data, size_t size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(size >= sizeof(HtcmiscRpcPacket));
        AMS_UNUSED(size);

        this->Complete(reinterpret_cast<const HtcmiscRpcPacket *>(data)->params[0]);
        return ResultSuccess();
    }

    os::SystemEventType *RunOnHostTask::GetSystemEvent() {
        return m_system_event.GetBase();
    }

}
