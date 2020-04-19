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

    namespace {

        constexpr const size_t LinebufferCount = 4;
        constexpr const size_t ColorComponents = 3;

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
        AMS_ASSERT(util::IsAligned(in.width, 0x10));
        AMS_ASSERT(util::IsAligned(in.height, 0x4));

        AMS_ASSERT(out.bmp != nullptr);
        AMS_ASSERT(out.bmp_size >= 4 * in.width * in.height);

        AMS_ASSERT(out.width != nullptr);
        AMS_ASSERT(out.height != nullptr);

        const size_t linebuffer_size = ColorComponents * in.width;
        const size_t total_linebuffer_size = LinebufferCount * linebuffer_size;
        R_UNLESS(work_size >= total_linebuffer_size, capsrv::ResultOutOfWorkMemory());

        jpeg_decompress_struct cinfo;
        std::memset(&cinfo, 0, sizeof(cinfo));

        jpeg_error_mgr jerr;
        std::memset(&jerr, 0, sizeof(jerr));

        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = JpegErrorExit;

        /* ? */

        jpeg_create_decompress(&cinfo);

        ON_SCOPE_EXIT {
            jpeg_destroy_decompress(&cinfo);
        };

        jpeg_mem_src(&cinfo, in.jpeg, in.jpeg_size);

        int res = jpeg_read_header(&cinfo, true);

        R_UNLESS(res == JPEG_HEADER_OK,             capsrv::ResultInvalidFileData());

        R_UNLESS(cinfo.image_width == in.width,     capsrv::ResultInvalidFileData());
        R_UNLESS(cinfo.image_height == in.height,   capsrv::ResultInvalidFileData());

        cinfo.out_color_space = JCS_RGB;
        cinfo.dct_method = JDCT_ISLOW;
        cinfo.do_fancy_upsampling = in.fancy_upsampling;
        cinfo.do_block_smoothing = in.block_smoothing;

        res = jpeg_start_decompress(&cinfo);

        R_UNLESS(res == TRUE, capsrv::ResultInvalidFileData());

        R_UNLESS(cinfo.output_width == in.width,    capsrv::ResultInvalidArgument());
        R_UNLESS(cinfo.output_height == in.height,  capsrv::ResultInvalidArgument());

        R_UNLESS(cinfo.out_color_components == ColorComponents, capsrv::ResultInvalidArgument());
        R_UNLESS(cinfo.output_components == ColorComponents,    capsrv::ResultInvalidArgument());

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

            /* Done! */
            if (parsed == 0)
                break;
            
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

        res = jpeg_finish_decompress(&cinfo);
        R_UNLESS(res == TRUE, capsrv::ResultInvalidFileData());

        *out.width = cinfo.output_width;
        *out.height = cinfo.output_height;

        return ResultSuccess();
    }

}