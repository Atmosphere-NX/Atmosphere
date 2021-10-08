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
#include "cs_command_impl.hpp"

/* Include command implementations. */
#include "cs_command_impl.inc"

namespace ams::cs {

    namespace {

        struct ResponseFirmwareVersion {
            ResponseHeader header;
            settings::system::FirmwareVersion firmware_version;
        };

        struct ResponseTakeScreenShot {
            ResponseHeader header;
            s32 data_size;
            s32 width;
            s32 height;
        };

        constinit u8 g_data[0x1000];

    }

    bool CommandProcessor::ProcessCommand(const CommandHeader &header, const u8 *body, s32 socket) {
        switch (header.command) {
            case Command_GetFirmwareVersion:
                SendFirmwareVersion(socket, header);
                break;
            case Command_TakeScreenShot:
                this->TakeScreenShot(header, socket, vi::LayerStack_ApplicationForDebug);
                break;
            case Command_TakeForegroundScreenShot:
                this->TakeScreenShot(header, socket, vi::LayerStack_LastFrame);
                break;
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

    void CommandProcessor::TakeScreenShot(const CommandHeader &header, s32 socket, vi::LayerStack layer_stack) {
        /* Create the command data. */
        const CommandDataTakeScreenShot params = {
            .layer_stack = layer_stack,
            .buffer      = g_data,
            .buffer_size = sizeof(g_data),
        };

        /* Take the screenshot. */
        Result result;
        {
            /* Acquire the send lock. */
            auto lk = MakeSendGuardBlock();

            /* Perform the command. */
            result = DoTakeScreenShotCommand(params, [&](s32 data_size, s32 width, s32 height) {
                /* Use global buffer for response. */
                ResponseTakeScreenShot *response = reinterpret_cast<ResponseTakeScreenShot *>(g_data);

                /* Set response header. */
                *response = {
                    .header    = {
                        .id = header.id,
                        .response  = Response_ScreenShot,
                        .body_size = static_cast<u32>(sizeof(data_size) + sizeof(width) + sizeof(height) + data_size),
                    },
                    .data_size = data_size,
                    .width     = width,
                    .height    = height,
                };

                /* Send data. */
                Send(socket, response, sizeof(*response));
            }, [&](u8 *data, size_t data_size) {
                /* Send data. */
                Send(socket, data, data_size);
            });
        }

        /* Handle the error case. */
        if (R_FAILED(result)) {
            SendErrorResult(socket, header, result);
        }
    }

}
