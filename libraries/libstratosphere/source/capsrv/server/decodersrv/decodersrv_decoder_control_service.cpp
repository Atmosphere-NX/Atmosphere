/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "decodersrv_decoder_server_object.hpp"
#include "../jpeg/decodersrv_software_jpeg_decoder.hpp"

namespace ams::capsrv::server {

    namespace {

        Result DecodeJpegImpl(void *dst, size_t dst_size, const void *src_jpeg, size_t src_jpeg_size, u32 width, u32 height, const ScreenShotDecodeOption &option, void *work, size_t work_size) {
            /* Clear the work memory. */
            std::memset(work, 0, work_size);
            ON_SCOPE_EXIT { std::memset(work, 0, work_size); };

            /* Clear the output memory. */
            std::memset(dst, 0, dst_size);
            auto clear_guard = SCOPE_GUARD { std::memset(dst, 0, dst_size); };

            /* Validate parameters. */
            R_UNLESS(util::IsAligned(width, 0x10),   capsrv::ResultAlbumOutOfRange());
            R_UNLESS(util::IsAligned(height, 0x4),   capsrv::ResultAlbumOutOfRange());

            R_UNLESS(dst != nullptr,                 capsrv::ResultAlbumReadBufferShortage());
            R_UNLESS(dst_size >= 4 * width * height, capsrv::ResultAlbumReadBufferShortage());

            R_UNLESS(src_jpeg != nullptr,            capsrv::ResultAlbumInvalidFileData());
            R_UNLESS(src_jpeg_size != 0,             capsrv::ResultAlbumInvalidFileData());

            /* Create the input. */
            const jpeg::SoftwareJpegDecoderInput decode_input = {
                .jpeg             = src_jpeg,
                .jpeg_size        = src_jpeg_size,
                .width            = width,
                .height           = height,
                .fancy_upsampling = option.HasJpegDecoderFlag(ScreenShotDecoderFlag_EnableFancyUpsampling),
                .block_smoothing  = option.HasJpegDecoderFlag(ScreenShotDecoderFlag_EnableBlockSmoothing),
            };

            /* Create the output. */
            s32 out_width = 0, out_height = 0;
            jpeg::SoftwareJpegDecoderOutput decode_output = {
                .out_width  = std::addressof(out_width),
                .out_height = std::addressof(out_height),
                .dst        = dst,
                .dst_size   = dst_size,
            };

            /* Decode the jpeg. */
            R_TRY(jpeg::SoftwareJpegDecoder::DecodeRgba8(decode_output, decode_input, work, work_size));

            /* We succeeded, so we shouldn't clear the output memory. */
            clear_guard.Cancel();
            return ResultSuccess();
        }

    }

    Result DecoderControlService::DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, u32 width, u32 height, const ScreenShotDecodeOption &option) {
        /* Get the work buffer. */
        void *work       = g_work_memory.jpeg_decoder_memory;
        size_t work_size = sizeof(g_work_memory.jpeg_decoder_memory);

        /* Call the decoder implementation. */
        return DecodeJpegImpl(out.GetPointer(), out.GetSize(), in.GetPointer(), in.GetSize(), width, height, option, work, work_size);
    }

}