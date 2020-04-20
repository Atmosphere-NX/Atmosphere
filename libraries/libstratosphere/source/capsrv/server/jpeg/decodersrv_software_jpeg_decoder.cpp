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
#include "decodersrv_software_jpeg_decoder.hpp"
#include "capsrv_server_jpeg_library_types.hpp"
#include "capsrv_server_jpeg_error_handler.hpp"


namespace ams::capsrv::server::jpeg {

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

        constexpr s32 ImageSizeHorizonalUnit  = 0x10;
        constexpr s32 ImageSizeVerticalUnit   = 0x4;

        constexpr s32 RgbColorComponentCount  = 3;
        constexpr s32 RgbaColorComponentCount = 4;

        Result GetRgbBufferSize(size_t *out_size, size_t *out_stride, s32 width, size_t work_size) {
            /* Calculate the space we need and verify we have enough. */
            const size_t rgb_width  = util::AlignUp(static_cast<size_t>(width), ImageSizeHorizonalUnit);
            const size_t rgb_stride = rgb_width * RgbColorComponentCount;
            const size_t rgb_size   = rgb_stride * ImageSizeVerticalUnit;
            R_UNLESS(work_size >= rgb_size, capsrv::ResultInternalJpegWorkMemoryShortage());

            /* Return the output to the caller. */
            *out_size   = rgb_size;
            *out_stride = rgb_stride;
            return ResultSuccess();
        }

    }

    Result SoftwareJpegDecoder::DecodeRgba8(SoftwareJpegDecoderOutput &output, const SoftwareJpegDecoderInput &input, void *work, size_t work_size) {
        CAPSRV_ABORT_UNLESS(util::IsAligned(input.width,  ImageSizeHorizonalUnit));
        CAPSRV_ABORT_UNLESS(util::IsAligned(input.height, ImageSizeVerticalUnit));

        CAPSRV_ABORT_UNLESS(output.dst != nullptr);
        CAPSRV_ABORT_UNLESS(output.dst_size >= static_cast<size_t>(RgbaColorComponentCount * input.width * input.height));

        CAPSRV_ABORT_UNLESS(output.out_width != nullptr);
        CAPSRV_ABORT_UNLESS(output.out_height != nullptr);

        /* Determine work buffer extents. */
        char *work_start = static_cast<char *>(work);
        char *work_end   = work_start + work_size;

        /* Determine the buffer extents for our linebuffers. */
        u8 *rgb_buffer = static_cast<u8 *>(static_cast<void *>(work_start));
        size_t rgb_buffer_size;
        size_t rgb_buffer_stride;
        R_TRY(GetRgbBufferSize(std::addressof(rgb_buffer_size), std::addressof(rgb_buffer_stride), input.width, work_size));

        /* The start of the workbuffer is reserved for linebuffer space. */
        work_start += rgb_buffer_size;

        /* Create our decompression structure. */
        JpegLibraryType::jpeg_decompress_struct cinfo = {};

        /* Here nintendo creates a work buffer structure containing work_start + work_size. */
        /* This seems to be a custom patch for/to libjpeg-turbo. */
        /* It would be desirable for us to mimic this, because it gives Nintendo strong */
        /* fixed memory usage guarantees. */
        /* TODO: Determine if it is feasible for us to recreate this ourselves, */
        /* Either by adding support to the devkitPro libjpeg-turbo portlib or otherwise. */
        AMS_UNUSED(work_end);

        /* Create our error manager. */
        JpegErrorHandler jerr = {
            .result     = ResultSuccess(),
        };
        jerr.error_exit = JpegErrorHandler::HandleError,


        /* Link our error manager to our decompression structure. */
        cinfo.err = jpeg_std_error(std::addressof(jerr));

        /* Use setjmp, so that on error our handler will longjmp to return an error result. */
        if (setjmp(jerr.jmp_buf) == 0) {
            /* Create our decompressor. */
            jpeg_create_decompress(std::addressof(cinfo));
            ON_SCOPE_EXIT { jpeg_destroy_decompress(std::addressof(cinfo)); };

            /* Setup our memory reader, ensure the header is correct. */
            jpeg_mem_src(std::addressof(cinfo), const_cast<unsigned char *>(static_cast<const unsigned char *>(input.jpeg)), input.jpeg_size);
            R_UNLESS(jpeg_read_header(std::addressof(cinfo), true) == JPEG_HEADER_OK,  capsrv::ResultAlbumInvalidFileData());

            /* Ensure width and height are correct. */
            R_UNLESS(cinfo.image_width  == input.width,    capsrv::ResultAlbumInvalidFileData());
            R_UNLESS(cinfo.image_height == input.height,   capsrv::ResultAlbumInvalidFileData());

            /* Set output parameters. */
            cinfo.out_color_space       = JpegLibraryType::J_COLOR_SPACE::JCS_RGB;
            cinfo.dct_method            = JpegLibraryType::J_DCT_METHOD::JDCT_ISLOW;
            cinfo.do_fancy_upsampling   = input.fancy_upsampling;
            cinfo.do_block_smoothing    = input.block_smoothing;

            /* Start decompression. */
            R_UNLESS(jpeg_start_decompress(&cinfo) == TRUE, capsrv::ResultAlbumInvalidFileData());

            /* Check the parameters. */
            CAPSRV_ASSERT(cinfo.output_width         == input.width);
            CAPSRV_ASSERT(cinfo.output_height        == input.height);
            CAPSRV_ASSERT(cinfo.out_color_components == RgbColorComponentCount);
            CAPSRV_ASSERT(cinfo.output_components    == RgbColorComponentCount);

            /* Parse the scanlines. */
            {
                /* Convert our destination to a writable u8 buffer. */
                u8 *dst = static_cast<u8 *>(output.dst);

                /* Create our linebuffer structure. */
                JpegLibraryType::JSAMPROW linebuffers[ImageSizeVerticalUnit] = {};
                for (int i = 0; i < ImageSizeVerticalUnit; i++) {
                    linebuffers[i] = rgb_buffer + rgb_buffer_stride * i;
                }

                /* While we still have scanlines, parse! */
                while (cinfo.output_scanline < input.height) {
                    /* Decode scanlines. */
                    int num_scanlines = jpeg_read_scanlines(std::addressof(cinfo), linebuffers, ImageSizeVerticalUnit);
                    CAPSRV_ASSERT(num_scanlines <= ImageSizeVerticalUnit);

                    /* Write out line by line. */
                    for (s32 i = 0; i < num_scanlines; i++) {
                        const u8 *src = linebuffers[i];
                        for (s32 j = 0; j < static_cast<s32>(input.width); j++) {
                            /* Write the output. */

                            /* First R, */
                            *(dst++) = *(src++);

                            /* Then G, */
                            *(dst++) = *(src++);

                            /* Then B, */
                            *(dst++) = *(src++);

                            /* Then A. */
                            *(dst++) = 0xFF;
                        }
                    }
                }
            }

            /* Finish the decompression. */
            R_UNLESS(jpeg_finish_decompress(&cinfo) == TRUE, capsrv::ResultAlbumInvalidFileData());
        } else {
            /* Some unknown error was caught by our handler. */
            return capsrv::ResultAlbumInvalidFileData();
        }

        /* Write the size we decoded to output. */
        *output.out_width = static_cast<s32>(cinfo.output_width);
        *output.out_width = static_cast<s32>(cinfo.output_height);

        return ResultSuccess();
    }

}
