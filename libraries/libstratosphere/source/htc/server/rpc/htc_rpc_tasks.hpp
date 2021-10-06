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

namespace ams::htc::server::rpc {

    constexpr inline size_t MaxRpcCount = 0x48;

    enum class PacketCategory : s16 {
        Request      = 0,
        Response     = 1,
        Notification = 2,
    };

    struct RpcPacket {
        s16 protocol;
        s16 version;
        PacketCategory category;
        u16 type;
        s64 body_size;
        u32 task_id;
        u64 params[5];
        char data[];
    };
    static_assert(sizeof(RpcPacket) == 0x40);

    enum class RpcTaskCancelReason {
        None              = 0,
        BySocket          = 1,
        ClientFinalized   = 2,
        QueueNotAvailable = 3,
    };

    enum class RpcTaskState {
        Started   = 0,
        Completed = 1,
        Cancelled = 2,
        Notified  = 3,
    };

    class Task {
        private:
            RpcTaskState m_state;
            RpcTaskCancelReason m_cancel_reason;
            os::Event m_event;
        public:
            Task() : m_state(RpcTaskState::Started), m_cancel_reason(RpcTaskCancelReason::None), m_event(os::EventClearMode_AutoClear) { /* ... */ }
            virtual ~Task() { /* ... */ }
        public:
            void SetTaskState(RpcTaskState state) { m_state = state; }
            RpcTaskState GetTaskState() const { return m_state; }

            RpcTaskCancelReason GetTaskCancelReason() const { return m_cancel_reason; }

            os::EventType *GetEvent() { return m_event.GetBase(); }

            void Notify() {
                AMS_ASSERT(m_state == RpcTaskState::Started);

                m_state = RpcTaskState::Notified;
            }

            void Complete() {
                AMS_ASSERT(m_state == RpcTaskState::Started || m_state == RpcTaskState::Notified);

                m_state = RpcTaskState::Completed;
                m_event.Signal();
            }
        public:
            virtual void Cancel(RpcTaskCancelReason reason) {
                m_state         = RpcTaskState::Cancelled;
                m_cancel_reason = reason;
                m_event.Signal();
            }

            virtual Result ProcessResponse(const char *data, size_t size) {
                AMS_UNUSED(data, size);
                return ResultSuccess();
            }

            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
                AMS_UNUSED(out, data, size, task_id);
                return ResultSuccess();
            }

            virtual Result ProcessNotification(const char *data, size_t size) {
                AMS_UNUSED(data, size);
                return ResultSuccess();
            }

            virtual Result CreateNotification(size_t *out, char *data, size_t size, u32 task_id) {
                AMS_UNUSED(out, data, size, task_id);
                return ResultSuccess();
            }

            virtual bool IsReceiveBufferRequired() {
                return false;
            }

            virtual bool IsSendBufferRequired() {
                return false;
            }

            virtual os::SystemEventType *GetSystemEvent() {
                return nullptr;
            }
    };

}
