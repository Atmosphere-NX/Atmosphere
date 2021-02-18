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

    class SocketTask : public HtcsTask {
        private:
            htcs::SocketError m_err;
            s32 m_desc;
        public:
            SocketTask() : HtcsTask(HtcsTaskType::Socket) { /* ... */ }

            Result SetArguments();
            void Complete(htcs::SocketError err, s32 desc);
            Result GetResult(htcs::SocketError *out_err, s32 *out_desc);
        public:
            virtual Result ProcessResponse(const char *data, size_t size) override;
            virtual Result CreateRequest(size_t *out, char *data, size_t size, u32 task_id) override;
    };

}
