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
#include "jpegdec_decode_service.hpp"
#include "impl/jpegdec_turbo.hpp"

namespace ams::jpegdec {

    namespace {

        /* Enough for four linebuffers RGB. */
        u8 g_workmem[0x3C00];

    }

    Result DecodeService::DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, u32 width, u32 height, const CapsScreenShotDecodeOption &opts) {
        u8 *bmp = out.GetPointer();
        size_t bmp_size = out.GetSize();
        const u8 *jpeg = in.GetPointer();
        size_t jpeg_size = in.GetSize();

        memset(g_workmem, 0, sizeof(g_workmem));
        memset(bmp, 0, bmp_size);

        R_UNLESS(util::IsAligned(width, 0x10),   capsrv::ResultAlbumOutOfRange());
        R_UNLESS(util::IsAligned(height, 0x4),   capsrv::ResultAlbumOutOfRange());

        R_UNLESS(bmp != nullptr,                 capsrv::ResultAlbumReadBufferShortage());
        R_UNLESS(bmp_size >= 4 * width * height, capsrv::ResultAlbumReadBufferShortage());

        R_UNLESS(jpeg != nullptr,                capsrv::ResultAlbumInvalidFileData());
        R_UNLESS(jpeg_size != 0,                 capsrv::ResultAlbumInvalidFileData());

        impl::DecodeInput decode_input = {
            .jpeg = jpeg,
            .jpeg_size = jpeg_size,
            .width = width,
            .height = height,
            .fancy_upsampling = bool(opts.fancy_upsampling),
            .block_smoothing = bool(opts.block_smoothing),
        };

        /* Official software ignores output written to this struct. */
        impl::Dimensions dims = {};

        impl::DecodeOutput decode_output = {
            .width = &dims.width,
            .height = &dims.height,
            .bmp = bmp,
            .bmp_size = bmp_size,
        };

        /* Clear output memory on decode failure. */
        /* NOTE: Nintendo does not do this. */
        auto clear_guard = SCOPE_GUARD { std::memset(bmp, 0, bmp_size); };

        /* Decode the jpeg. */
        R_TRY(impl::DecodeJpeg(decode_output, decode_input, g_workmem, sizeof(g_workmem)));
        clear_guard.Cancel();

        /* Clear the work memory. */
        /* NOTE: Nintendo does not do this. */
        std::memset(g_workmem, 0, sizeof(g_workmem));

        return ResultSuccess();
    }
}