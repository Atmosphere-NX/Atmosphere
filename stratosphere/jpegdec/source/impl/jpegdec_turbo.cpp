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
#include "jpegdec_turbo.hpp"

#include <jpeglib.h>

namespace ams::jpegdec::impl {

    #define CAPSRV_ABORT_UNLESS(expr) do {                          \
        const bool __capsrv_assert_res = (expr);                    \
        AMS_ASSERT(__capsrv_assert_res);                            \
        AMS_ABORT_UNLESS(__capsrv_assert_res);                      \
    } while (0)

    #define CAPSRV_ASSERT(expr) do {                                \
        const bool __capsrv_assert_res = (expr);                    \
        AMS_ASSERT(__capsrv_assert_res);                            \
        R_UNLESS(__capsrv_assert_res, capsrv::ResultAlbumError());  \
    } while (0)

    namespace {

        constexpr size_t LinebufferCount = 4;
        constexpr size_t ColorComponents = 3;

        constexpr int ImageSizeHorizonalUnit = 0x10;
        constexpr int ImageSizeVerticalUnit  = 0x4;

        struct RGB {
            u8 r, g, b;
        };

        struct RGBX {
            u8 r, g, b, x;
        };

        void JpegErrorExit(j_common_ptr cinfo) {
            /* ? */
        }
    }

    Result DecodeJpeg(DecodeOutput &out, const DecodeInput &in, u8 *work, size_t work_size) {
        CAPSRV_ABORT_UNLESS(util::IsAligned(in.width,  ImageSizeHorizonalUnit));
        CAPSRV_ABORT_UNLESS(util::IsAligned(in.height, ImageSizeVerticalUnit));

        CAPSRV_ABORT_UNLESS(out.bmp != nullptr);
        CAPSRV_ABORT_UNLESS(out.bmp_size >= 4 * in.width * in.height);

        CAPSRV_ABORT_UNLESS(out.width != nullptr);
        CAPSRV_ABORT_UNLESS(out.height != nullptr);

        const size_t linebuffer_size = ColorComponents * in.width;
        const size_t total_linebuffer_size = LinebufferCount * linebuffer_size;
        R_UNLESS(work_size >= total_linebuffer_size, capsrv::ResultInternalJpegWorkMemoryShortage());

        jpeg_decompress_struct cinfo;
        std::memset(&cinfo, 0, sizeof(cinfo));

        jpeg_error_mgr jerr;
        std::memset(&jerr, 0, sizeof(jerr));

        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = JpegErrorExit;

        /* TODO: Here Nintendo uses setjmp, on longjmp to error ResultAlbumInvalidFileData is returned.  */

        jpeg_create_decompress(&cinfo);

        ON_SCOPE_EXIT {
            jpeg_destroy_decompress(&cinfo);
        };

        jpeg_mem_src(&cinfo, in.jpeg, in.jpeg_size);

        R_UNLESS(jpeg_read_header(&cinfo, true) == JPEG_HEADER_OK,  capsrv::ResultAlbumInvalidFileData());

        R_UNLESS(cinfo.image_width  == in.width,    capsrv::ResultAlbumInvalidFileData());
        R_UNLESS(cinfo.image_height == in.height,   capsrv::ResultAlbumInvalidFileData());

        cinfo.out_color_space = JCS_RGB;
        cinfo.dct_method = JDCT_ISLOW;
        cinfo.do_fancy_upsampling = in.fancy_upsampling;
        cinfo.do_block_smoothing = in.block_smoothing;

        R_UNLESS(jpeg_start_decompress(&cinfo) == TRUE, capsrv::ResultAlbumInvalidFileData());

        CAPSRV_ASSERT(cinfo.output_width         == in.width);
        CAPSRV_ASSERT(cinfo.output_height        == in.height);
        CAPSRV_ASSERT(cinfo.out_color_components == ColorComponents);
        CAPSRV_ASSERT(cinfo.output_components    == ColorComponents);

        /* Pointer to output. */
        RGBX *bmp = reinterpret_cast<RGBX *>(out.bmp);

        /* Decode 4 lines at once. */
        u8 *linebuffer[4] = {
            work + 0 * linebuffer_size,
            work + 1 * linebuffer_size,
            work + 2 * linebuffer_size,
            work + 3 * linebuffer_size,
        };

        /* While we still have scanlines, parse! */
        while (cinfo.output_scanline < cinfo.output_height) {
            /* Decode scanlines. */
            int parsed = jpeg_read_scanlines(&cinfo, linebuffer, 4);
            CAPSRV_ASSERT(parsed <= ImageSizeVerticalUnit);

            /* Line by line */
            for (int index = 0; index < parsed; index++) {
                u8 *buffer = linebuffer[index];
                const RGB* rgb = reinterpret_cast<RGB *>(buffer);
                for (u32 i = 0; i < in.width; i++) {
                    /* Fill output. */
                    bmp->r = rgb->r;
                    bmp->g = rgb->g;
                    bmp->b = rgb->b;
                    bmp->x = 0xFF;

                    /* Traverse buffer. */
                    bmp++;
                    rgb++;
                }
            }
        }

        R_UNLESS(jpeg_finish_decompress(&cinfo) == TRUE, capsrv::ResultAlbumInvalidFileData());

        *out.width = cinfo.output_width;
        *out.height = cinfo.output_height;

        return ResultSuccess();
    }

}