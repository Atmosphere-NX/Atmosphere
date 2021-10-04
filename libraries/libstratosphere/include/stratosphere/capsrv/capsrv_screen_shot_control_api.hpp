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
#include <vapours.hpp>
#include <stratosphere/vi/vi_layer_stack.hpp>

namespace ams::capsrv {

    constexpr inline s32 DefaultCaptureTimeoutMilliSeconds = 100;

    Result InitializeScreenShotControl();
    void   FinalizeScreenShotControl();

    Result OpenRawScreenShotReadStreamForDevelop(size_t *out_data_size, s32 *out_width, s32 *out_height, vi::LayerStack layer_stack, TimeSpan timeout);
    Result ReadRawScreenShotReadStreamForDevelop(size_t *out_read_size, void *dst, size_t dst_size, std::ptrdiff_t offset);
    void CloseRawScreenShotReadStreamForDevelop();

    Result CaptureJpegScreenshot(u64 *out_size, void *dst, size_t dst_size, vi::LayerStack layer_stack, TimeSpan timeout);

}
