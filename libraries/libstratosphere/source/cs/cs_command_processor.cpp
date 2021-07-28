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
#include "cs_command_impl.hpp"

namespace ams::cs {

    namespace {

        struct ResponseFirmwareVersion {
            ResponseHeader header;
            settings::system::FirmwareVersion firmware_version;
        };

    }

    bool CommandProcessor::ProcessCommand(const CommandHeader &header, const u8 *body, s32 socket) {
        switch (header.command) {
            case Command_GetFirmwareVersion:
                SendFirmwareVersion(socket, header);
                break;
            /* TODO: Command support. */
            /* TODO: Command support. */
            default:
                scs::CommandProcessor::ProcessCommand(header, body, socket);
                break;
        }

        return true;
    }

    void CommandProcessor::SendFirmwareVersion(s32 socket, const CommandHeader &header) {
        /* Build the response. */
        ResponseFirmwareVersion response = {
            .header = {
                .id        = header.id,
                .response  = Response_FirmwareVersion,
                .body_size = sizeof(response) - sizeof(response.header),
            },
            .firmware_version = {},
        };

        /* Get the firmware version. */
        const Result result = DoGetFirmwareVersionCommand(std::addressof(response.firmware_version));
        if (R_SUCCEEDED(result)) {
            /* Send the response. */
            auto lk = MakeSendGuardBlock();
            Send(socket, std::addressof(response), sizeof(response));
        } else {
            SendErrorResult(socket, header, result);
        }
    }

}
