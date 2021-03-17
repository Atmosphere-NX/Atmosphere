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

namespace ams::scs {

    namespace {

        struct ResponseError {
            ResponseHeader header;
            u32 result;
        };

        constinit os::SdkMutex g_htcs_send_mutex;

    }

    os::SdkMutex &GetHtcsSendMutex() {
        return g_htcs_send_mutex;
    }

    void CommandProcessor::SendSuccess(s32 socket, const CommandHeader &header) {
        /* Build the response. */
        const ResponseHeader response = {
            .id        = header.id,
            .response  = Response_Success,
            .body_size = 0,
        };

        /* Send the response. */
        std::scoped_lock lk(GetHtcsSendMutex());
        htcs::Send(socket, std::addressof(response), sizeof(response), 0);
    }

    void CommandProcessor::SendErrorResult(s32 socket, const CommandHeader &header, Result result) {
        return SendErrorResult(socket, header.id, result);
    }

    void CommandProcessor::SendErrorResult(s32 socket, u64 id, Result result) {
        /* Build the response. */
        const ResponseError response = {
            .header = {
                .id        = id,
                .response  = Response_Error,
                .body_size = sizeof(response) - sizeof(response.header),
            },
            .result = result.GetValue(),
        };

        /* Send the response. */
        std::scoped_lock lk(GetHtcsSendMutex());
        htcs::Send(socket, std::addressof(response), sizeof(response), 0);
    }

    void CommandProcessor::OnProcessStart(u64 id, s32 socket, os::ProcessId process_id) {
        /* TODO */
        AMS_ABORT("CommandProcessor::OnProcessStart");
    }

    void CommandProcessor::OnProcessExit(u64 id, s32 socket, os::ProcessId process_id) {
        /* TODO */
        AMS_ABORT("CommandProcessor::OnProcessExit");
    }

    void CommandProcessor::OnProcessJitDebug(u64 id, s32 socket, os::ProcessId process_id) {
        /* TODO */
        AMS_ABORT("CommandProcessor::OnProcessJitDebug");
    }

    void CommandProcessor::Initialize() {
        /* Register our process event handlers. */
        scs::RegisterCommonProcessEventHandler(OnProcessStart, OnProcessExit, OnProcessJitDebug);
    }

    bool CommandProcessor::ProcessCommand(const CommandHeader &header, const u8 *body, s32 socket) {
        switch (header.command) {
            /* TODO: Support commands. */
            default:
                SendErrorResult(socket, header, scs::ResultUnknownCommand());
                break;
        }

        return true;
    }

}
