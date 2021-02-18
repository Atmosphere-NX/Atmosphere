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
#include <stratosphere.hpp>
#include "../../../htc/server/rpc/htc_rpc_tasks.hpp"

namespace ams::htcs::impl::rpc {

    enum class HtcsTaskType {
        Receive      = 0,
        Send         = 1,
        Shutdown     = 2,
        Close        = 3,
        Connect      = 4,
        Listen       = 5,
        Accept       = 6,
        Socket       = 7,
        Bind         = 8,
        Fcntl        = 9,
        ReceiveSmall = 10,
        SendSmall    = 11,
        Select       = 12,
    };

    constexpr inline const s16 ProtocolVersion = 4;

    class HtcsTask : public htc::server::rpc::Task {
        private:
            HtcsTaskType m_task_type;
            s16 m_version;
        public:
            HtcsTask(HtcsTaskType type) : m_task_type(type), m_version(ProtocolVersion) { /* ... */ }

            HtcsTaskType GetTaskType() const { return m_task_type; }
            s16 GetVersion() const { return m_version; }
    };

    class HtcsSignalingTask : public HtcsTask {
        private:
            os::SystemEventType m_system_event;
            bool m_is_valid;
        public:
            HtcsSignalingTask(HtcsTaskType type);
            virtual ~HtcsSignalingTask();

            bool IsValid() const { return m_is_valid; }

            void Complete() {
                os::SignalSystemEvent(std::addressof(m_system_event));
                HtcsTask::Complete();
            }
        public:
            virtual void Cancel(htc::server::rpc::RpcTaskCancelReason reason) override {
                HtcsTask::Cancel(reason);
                os::SignalSystemEvent(std::addressof(m_system_event));
            }

            virtual os::SystemEventType *GetSystemEvent() override { return std::addressof(m_system_event); }
    };

    class ReceiveTask : public HtcsSignalingTask {
        private:
            s32 m_handle;
            s64 m_size;
            htcs::MessageFlag m_flags;
            void *m_buffer;
            s64 m_buffer_size;
            htcs::SocketError m_err;
            s64 m_result_size;
        public:
            ReceiveTask() : HtcsSignalingTask(HtcsTaskType::Receive) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s64 GetSize() const { return m_size; }
            htcs::MessageFlag GetFlags() const { return m_flags; }
            void *GetBuffer() const { return m_buffer; }
            s64 GetBufferSize() const { return m_buffer_size; }

            s64 GetResultSize() const {
                AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);
                return m_result_size;
            }
        public:
            Result SetArguments(s32 handle, s64 size, htcs::MessageFlag flags);
            void Complete(htcs::SocketError err, s64 size);
            Result GetResult(htcs::SocketError *out_err, s64 *out_size) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
            virtual Result CreateNotification(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class SendTask : public HtcsSignalingTask {
        private:
            os::Event m_ready_event;
            s32 m_handle;
            s64 m_size;
            htcs::MessageFlag m_flags;
            void *m_buffer;
            s64 m_buffer_size;
            htcs::SocketError m_err;
            s64 m_result_size;
        public:
            SendTask() : HtcsSignalingTask(HtcsTaskType::Send), m_ready_event(os::EventClearMode_ManualClear) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s64 GetSize() const { return m_size; }
            htcs::MessageFlag GetFlags() const { return m_flags; }
            void *GetBuffer() const { return m_buffer; }
            s64 GetBufferSize() const { return m_buffer_size; }

            void SetBuffer(const void *buffer, s64 buffer_size);
            void NotifyDataChannelReady();
            void WaitNotification();
        public:
            Result SetArguments(s32 handle, s64 size, htcs::MessageFlag flags);
            void Complete(htcs::SocketError err, s64 size);
            Result GetResult(htcs::SocketError *out_err, s64 *out_size) const;
        public:
            virtual void Cancel(htc::server::rpc::RpcTaskCancelReason reason) override;
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
            virtual Result ProcessNotification(const char *data, size_t size) override;
            virtual Result CreateNotification(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class ShutdownTask : public HtcsTask {
        private:
            s32 m_handle;
            ShutdownType m_how;
            htcs::SocketError m_err;
        public:
            ShutdownTask() : HtcsTask(HtcsTaskType::Shutdown) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            ShutdownType GetHow() const { return m_how; }
        public:
            Result SetArguments(s32 handle, ShutdownType how);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class CloseTask : public HtcsTask {
        private:
            s32 m_handle;
            htcs::SocketError m_err;
        public:
            CloseTask() : HtcsTask(HtcsTaskType::Close) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
        public:
            Result SetArguments(s32 handle);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class ConnectTask : public HtcsTask {
        private:
            s32 m_handle;
            HtcsPeerName m_peer_name;
            HtcsPortName m_port_name;
            htcs::SocketError m_err;
        public:
            ConnectTask() : HtcsTask(HtcsTaskType::Connect) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            const HtcsPeerName &GetPeerName() const { return m_peer_name; }
            const HtcsPortName &GetPortName() const { return m_port_name; }
        public:
            Result SetArguments(s32 handle, const HtcsPeerName &peer_name, const HtcsPortName &port_name);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class ListenTask : public HtcsTask {
        private:
            s32 m_handle;
            s32 m_backlog;
            htcs::SocketError m_err;
        public:
            ListenTask() : HtcsTask(HtcsTaskType::Listen) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s32 GetBacklog() const { return m_backlog; }
        public:
            Result SetArguments(s32 handle, s32 backlog);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class AcceptTask : public HtcsSignalingTask {
        private:
            s32 m_server_handle;
            htcs::SocketError m_err;
            s32 m_desc;
        public:
            AcceptTask() : HtcsSignalingTask(HtcsTaskType::Accept) { /* ... */ }

            s32 GetServerHandle() const { return m_server_handle; }
        public:
            Result SetArguments(s32 server_handle);
            void Complete(htcs::SocketError err, s32 desc);
            Result GetResult(htcs::SocketError *out_err, s32 *out_desc, s32 server_handle) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class SocketTask : public HtcsTask {
        private:
            htcs::SocketError m_err;
            s32 m_desc;
        public:
            SocketTask() : HtcsTask(HtcsTaskType::Socket) { /* ... */ }
        public:
            Result SetArguments();
            void Complete(htcs::SocketError err, s32 desc);
            Result GetResult(htcs::SocketError *out_err, s32 *out_desc) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class BindTask : public HtcsTask {
        private:
            s32 m_handle;
            HtcsPeerName m_peer_name;
            HtcsPortName m_port_name;
            htcs::SocketError m_err;
        public:
            BindTask() : HtcsTask(HtcsTaskType::Bind) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            const HtcsPeerName &GetPeerName() const { return m_peer_name; }
            const HtcsPortName &GetPortName() const { return m_port_name; }
        public:
            Result SetArguments(s32 handle, const HtcsPeerName &peer_name, const HtcsPortName &port_name);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class FcntlTask : public HtcsTask {
        private:
            s32 m_handle;
            s32 m_command;
            s32 m_value;
            htcs::SocketError m_err;
            s32 m_res;
        public:
            FcntlTask() : HtcsTask(HtcsTaskType::Fcntl) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s32 GetCommand() const { return m_command; }
            s32 GetValue() const { return m_value; }
        public:
            Result SetArguments(s32 handle, s32 command, s32 value);
            void Complete(htcs::SocketError err);
            Result GetResult(htcs::SocketError *out_err, s32 *out_res) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

    class ReceiveSmallTask : public HtcsSignalingTask {
        private:
            s32 m_handle;
            s64 m_size;
            htcs::MessageFlag m_flags;
            char m_buffer[0xE000];
            htcs::SocketError m_err;
            s64 m_result_size;
        public:
            ReceiveSmallTask() : HtcsSignalingTask(HtcsTaskType::ReceiveSmall) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s64 GetSize() const { return m_size; }
            htcs::MessageFlag GetFlags() const { return m_flags; }
            void *GetBuffer() { return m_buffer; }
            s64 GetBufferSize() const { return static_cast<s64>(sizeof(m_buffer)); }

            s64 GetResultSize() const {
                AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);
                return m_result_size;
            }
        public:
            Result SetArguments(s32 handle, s64 size, htcs::MessageFlag flags);
            void Complete(htcs::SocketError err, s64 size);
            Result GetResult(htcs::SocketError *out_err, s64 *out_size) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
            virtual bool IsReceiveBufferRequired() override;
    };

    class SendSmallTask : public HtcsSignalingTask {
        private:
            os::Event m_ready_event;
            s32 m_handle;
            s64 m_size;
            htcs::MessageFlag m_flags;
            char m_buffer[0xE000];
            s64 m_buffer_size;
            htcs::SocketError m_err;
            s64 m_result_size;
        public:
            SendSmallTask() : HtcsSignalingTask(HtcsTaskType::SendSmall), m_ready_event(os::EventClearMode_ManualClear) { /* ... */ }

            s32 GetHandle() const { return m_handle; }
            s64 GetSize() const { return m_size; }
            htcs::MessageFlag GetFlags() const { return m_flags; }
            void *GetBuffer() { return m_buffer; }
            s64 GetBufferSize() const { return m_buffer_size; }

            void SetBuffer(const void *buffer, s64 buffer_size);
            void NotifyDataChannelReady();
            void WaitNotification();
        public:
            Result SetArguments(s32 handle, s64 size, htcs::MessageFlag flags);
            void Complete(htcs::SocketError err, s64 size);
            Result GetResult(htcs::SocketError *out_err, s64 *out_size) const;
        public:
            virtual void Cancel(htc::server::rpc::RpcTaskCancelReason reason) override;
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
            virtual bool IsSendBufferRequired() override;
    };

    class SelectTask : public HtcsSignalingTask {
        private:
            s32 m_handles[SocketCountMax * 3];
            s32 m_read_handle_count;
            s32 m_write_handle_count;
            s32 m_exception_handle_count;
            s64 m_tv_sec;
            s64 m_tv_usec;
            htcs::SocketError m_err;
            s32 m_out_handles[SocketCountMax * 3];
            s32 m_out_read_handle_count;
            s32 m_out_write_handle_count;
            s32 m_out_exception_handle_count;
        public:
            SelectTask() : HtcsSignalingTask(HtcsTaskType::Select) { /* ... */ }

            const s32 *GetHandles() const { return m_handles; }
            s32 GetReadHandleCount() const { return m_read_handle_count; }
            s32 GetWriteHandleCount() const { return m_write_handle_count; }
            s32 GetExceptionHandleCount() const { return m_exception_handle_count; }
            s64 GetTimeoutSeconds() const { return m_tv_sec; }
            s64 GetTimeoutMicroSeconds() const { return m_tv_usec; }
        public:
            Result SetArguments(Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec);
            void Complete(htcs::SocketError err, s32 read_handle_count, s32 write_handle_count, s32 exception_handle_count, const void *body, s64 body_size);
            Result GetResult(htcs::SocketError *out_err, bool *out_empty, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles) const;
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

}
