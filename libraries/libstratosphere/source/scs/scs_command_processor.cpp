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

namespace ams::scs {

    namespace {

        struct ResponseError {
            ResponseHeader header;
            u32 result;
        };

        struct ResponseProgramExited {
            ResponseHeader header;
            u64 process_id;
        };

        struct ResponseProgramLaunched {
            ResponseHeader header;
            u64 process_id;
        };

        constinit os::SdkMutex g_htcs_send_mutex;

    }

    std::scoped_lock<os::SdkMutex> CommandProcessor::MakeSendGuardBlock() {
        return std::scoped_lock<os::SdkMutex>{g_htcs_send_mutex};
    }

    void CommandProcessor::Send(s32 socket, const void *data, size_t size) {
        htcs::Send(socket, data, size, 0);
    }

    void CommandProcessor::SendSuccess(s32 socket, const CommandHeader &header) {
        /* Build the response. */
        const ResponseHeader response = {
            .id        = header.id,
            .response  = Response_Success,
            .body_size = 0,
        };

        /* Send the response. */
        auto lk = MakeSendGuardBlock();
        Send(socket, std::addressof(response), sizeof(response));
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
        auto lk = MakeSendGuardBlock();
        Send(socket, std::addressof(response), sizeof(response));
    }

    void CommandProcessor::SendExited(s32 socket, u64 id, u64 process_id) {
        /* Build the response. */
        const ResponseProgramExited response = {
            .header = {
                .id        = id,
                .response  = Response_ProgramExited,
                .body_size = sizeof(response) - sizeof(response.header),
            },
            .process_id = process_id,
        };

        /* Send the response. */
        auto lk = MakeSendGuardBlock();
        Send(socket, std::addressof(response), sizeof(response));
    }

    void CommandProcessor::SendJitDebug(s32 socket, u64 id) {
        /* Build the response. */
        const ResponseHeader response = {
            .id        = id,
            .response  = Response_JitDebug,
            .body_size = 0,
        };

        /* Send the response. */
        auto lk = MakeSendGuardBlock();
        Send(socket, std::addressof(response), sizeof(response));
    }

    void CommandProcessor::SendLaunched(s32 socket, u64 id, u64 process_id) {
        /* Build the response. */
        const ResponseProgramLaunched response = {
            .header = {
                .id        = id,
                .response  = Response_ProgramLaunched,
                .body_size = sizeof(response) - sizeof(response.header),
            },
            .process_id = process_id,
        };

        /* Send the response. */
        auto lk = MakeSendGuardBlock();
        Send(socket, std::addressof(response), sizeof(response));
    }

    void CommandProcessor::OnProcessStart(u64 id, s32 socket, os::ProcessId process_id) {
        SendLaunched(socket, id, process_id.value);
    }

    void CommandProcessor::OnProcessExit(u64 id, s32 socket, os::ProcessId process_id) {
        SendExited(socket, id, process_id.value);
    }

    void CommandProcessor::OnProcessJitDebug(u64 id, s32 socket, os::ProcessId process_id) {
        AMS_UNUSED(process_id);

        SendJitDebug(socket, id);
    }

    void CommandProcessor::Initialize() {
        /* Register our process event handlers. */
        scs::RegisterCommonProcessEventHandler(OnProcessStart, OnProcessExit, OnProcessJitDebug);
    }

    bool CommandProcessor::ProcessCommand(const CommandHeader &header, const u8 *body, s32 socket) {
        AMS_UNUSED(body);

        switch (header.command) {
            /* TODO: Support commands. */
            default:
                SendErrorResult(socket, header, scs::ResultUnknownCommand());
                break;
        }

        return true;
    }

}
