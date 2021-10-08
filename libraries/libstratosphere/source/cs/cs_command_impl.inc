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

namespace ams::cs {

    namespace {

        template<typename SendData>
        void SendEmptyData(const CommandDataTakeScreenShot &params, size_t remaining_size, SendData send_data) {
            /* Clear the data buffer. */
            std::memset(params.buffer, 0, params.buffer_size);

            /* Send data until the end. */
            while (remaining_size > 0) {
                /* Send as much as we can. */
                const auto cur_size = std::min(remaining_size, params.buffer_size);
                send_data(params.buffer, cur_size);

                /* Advance. */
                remaining_size -= cur_size;
            }
        }

    }

    Result DoGetFirmwareVersionCommand(settings::system::FirmwareVersion *out) {
        settings::system::GetFirmwareVersion(out);
        return ResultSuccess();
    }

    template<typename SendHeader, typename SendData>
    Result DoTakeScreenShotCommand(const CommandDataTakeScreenShot &params, SendHeader send_header, SendData send_data) {
        /* Initialize screenshot control. */
        R_TRY(capsrv::InitializeScreenShotControl());

        /* Finalize screenshot control when we're done. */
        ON_SCOPE_EXIT { capsrv::FinalizeScreenShotControl(); };

        /* Open screenshot read stream. */
        size_t data_size;
        s32 width, height;
        R_TRY(capsrv::OpenRawScreenShotReadStreamForDevelop(std::addressof(data_size), std::addressof(width), std::addressof(height), params.layer_stack, TimeSpan::FromSeconds(10)));

        /* Close the screenshot stream when we're done. */
        ON_SCOPE_EXIT { capsrv::CloseRawScreenShotReadStreamForDevelop(); };

        /* Send the header. */
        send_header(static_cast<s32>(data_size), width, height);

        /* Read and send data. */
        size_t total_read_size = 0;
        auto data_guard = SCOPE_GUARD { SendEmptyData(params, data_size - total_read_size, send_data); };
        while (total_read_size < data_size) {
            /* Read data from the stream. */
            size_t read_size;
            R_TRY(capsrv::ReadRawScreenShotReadStreamForDevelop(std::addressof(read_size), params.buffer, params.buffer_size, total_read_size));

            /* Send the data that was read. */
            send_data(params.buffer, read_size);

            /* Advance. */
            total_read_size += read_size;
        }
        data_guard.Cancel();

        return ResultSuccess();
    }

}
