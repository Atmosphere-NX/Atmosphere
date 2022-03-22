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

namespace ams::capsrv {

    Result InitializeScreenShotControl() {
        return ::capsscInitialize();
    }

    void FinalizeScreenShotControl() {
        return ::capsscExit();
    }

    Result CaptureJpegScreenshot(u64 *out_size, void *dst, size_t dst_size, vi::LayerStack layer_stack, TimeSpan timeout) {
        return ::capsscCaptureJpegScreenShot(out_size, dst, dst_size, static_cast<::ViLayerStack>(layer_stack), timeout.GetNanoSeconds());
    }

    Result OpenRawScreenShotReadStreamForDevelop(size_t *out_data_size, s32 *out_width, s32 *out_height, vi::LayerStack layer_stack, TimeSpan timeout) {
        u64 data_size, width, height;
        R_TRY(::capsscOpenRawScreenShotReadStream(std::addressof(data_size), std::addressof(width), std::addressof(height), static_cast<::ViLayerStack>(layer_stack), timeout.GetNanoSeconds()));

        *out_data_size = static_cast<size_t>(data_size);
        *out_width     = static_cast<s32>(width);
        *out_height    = static_cast<s32>(height);

        return ResultSuccess();
    }

    Result ReadRawScreenShotReadStreamForDevelop(size_t *out_read_size, void *dst, size_t dst_size, std::ptrdiff_t offset) {
        u64 read_size;
        R_TRY(::capsscReadRawScreenShotReadStream(std::addressof(read_size), dst, dst_size, static_cast<u64>(offset)));

        *out_read_size = static_cast<size_t>(read_size);
        return ResultSuccess();
    }

    void CloseRawScreenShotReadStreamForDevelop() {
        ::capsscCloseRawScreenShotReadStream();
    }

}
