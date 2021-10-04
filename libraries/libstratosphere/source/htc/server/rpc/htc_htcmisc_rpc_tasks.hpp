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
#include <stratosphere.hpp>
#include "htc_rpc_tasks.hpp"

namespace ams::htc::server::rpc {

    enum class HtcmiscTaskType {
        GetEnvironmentVariable       = 0,
        GetEnvironmentVariableLength = 1,
        SetTargetStatus              = 2,
        RunOnHost                    = 3,
    };

    enum class HtcmiscResult {
        Success            = 0,
        UnknownError       = 1,
        UnsupportedVersion = 2,
        InvalidRequest     = 3,
    };

    enum class HtcmiscPacketCategory : s16 {
        Request  = 0,
        Response = 1,
    };

    enum class HtcmiscPacketType : s16 {
        GetMaxProtocolVersion        =  0,
        SetProtocolVersion           =  1,
        GetEnvironmentVariable       = 16,
        GetEnvironmentVariableLength = 17,
        SetTargetStatus              = 18,
        RunOnHost                    = 19,
        GetWorkingDirectory          = 20,
        GetWorkingDirectorySize      = 21,
        SetTargetName                = 22,
    };

    constexpr inline s16 HtcmiscProtocol   = 4;
    constexpr inline s16 HtcmiscMaxVersion = 2;

    struct HtcmiscRpcPacket {
        s16 protocol;
        s16 version;
        HtcmiscPacketCategory category;
        HtcmiscPacketType type;
        s64 body_size;
        u32 task_id;
        u64 params[5];
        char data[];
    };
    static_assert(sizeof(HtcmiscRpcPacket) == 0x40);

    class HtcmiscTask : public Task {
        private:
            HtcmiscTaskType m_task_type;
        public:
            HtcmiscTask(HtcmiscTaskType type) : m_task_type(type) { /* ... */ }

            HtcmiscTaskType GetTaskType() const { return m_task_type; }
    };

    class GetEnvironmentVariableTask : public HtcmiscTask {
        public:
            static constexpr inline HtcmiscTaskType TaskType = HtcmiscTaskType::GetEnvironmentVariable;
        private:
            char m_name[0x800];
            int m_name_size;
            Result m_result;
            size_t m_value_size;
            char m_value[0x8000];
        public:
            GetEnvironmentVariableTask() : HtcmiscTask(HtcmiscTaskType::GetEnvironmentVariable) { /* ... */ }

            Result SetArguments(const char *args, size_t size);
            void Complete(HtcmiscResult result, const char *data, size_t size);
            Result GetResult(size_t *out, char *dst, size_t size) const;

            const char *GetName() const { return m_name; }
            int GetNameSize() const { return m_name_size; }
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class GetEnvironmentVariableLengthTask : public HtcmiscTask {
        public:
            static constexpr inline HtcmiscTaskType TaskType = HtcmiscTaskType::GetEnvironmentVariableLength;
        private:
            char m_name[0x800];
            int m_name_size;
            Result m_result;
            size_t m_value_size;
        public:
            GetEnvironmentVariableLengthTask() : HtcmiscTask(HtcmiscTaskType::GetEnvironmentVariableLength) { /* ... */ }

            Result SetArguments(const char *args, size_t size);
            void Complete(HtcmiscResult result, const char *data, size_t size);
            Result GetResult(size_t *out) const;

            const char *GetName() const { return m_name; }
            int GetNameSize() const { return m_name_size; }
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class RunOnHostTask : public HtcmiscTask {
        public:
            static constexpr inline HtcmiscTaskType TaskType = HtcmiscTaskType::RunOnHost;
        private:
            char m_command[0x2000];
            int m_command_size;
            Result m_result;
            int m_host_result;
            os::SystemEvent m_system_event;
        public:
            RunOnHostTask() : HtcmiscTask(HtcmiscTaskType::RunOnHost), m_system_event(os::EventClearMode_ManualClear, true) { /* ... */ }

            Result SetArguments(const char *args, size_t size);
            void Complete(int host_result);
            Result GetResult(int *out) const;

            const char *GetCommand() const { return m_command; }
            int GetCommandSize() const { return m_command_size; }
        public:
            virtual void Cancel(RpcTaskCancelReason reason) override;
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
            virtual os::SystemEventType *GetSystemEvent() override;
    };

}
