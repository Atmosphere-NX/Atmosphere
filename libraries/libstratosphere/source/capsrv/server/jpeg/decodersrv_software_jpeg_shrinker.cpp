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
#include "decodersrv_software_jpeg_shrinker.hpp"
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

        constexpr s32 ImageSizeHorizonalUnit      = 0x10;
        constexpr s32 ImageSizeVerticalUnitDecode = 0x4;
        constexpr s32 ImageSizeVerticalUnitEncode = 0x8;

        constexpr s32 RgbColorComponentCount  = 3;
        constexpr s32 RgbaColorComponentCount = 4;

        Result GetRgbBufferSize(size_t *out_size, size_t *out_stride, s32 width, size_t work_size) {
            /* Calculate the space we need and verify we have enough. */
            const size_t rgb_width  = util::AlignUp(static_cast<size_t>(width), ImageSizeHorizonalUnit);
            const size_t rgb_stride = rgb_width * RgbColorComponentCount;
            const size_t rgb_size   = rgb_stride * ImageSizeVerticalUnitEncode;
            R_UNLESS(work_size >= rgb_size, capsrv::ResultInternalJpegWorkMemoryShortage());

            /* Return the output to the caller. */
            *out_size   = rgb_size;
            *out_stride = rgb_stride;
            R_SUCCEED();
        }

        void SetupEncodingParameter(JpegLibraryType::jpeg_compress_struct *cinfo, const SoftwareJpegShrinkerInput &input, int quality) {
            /* Set parameters. */
            cinfo->image_width      = static_cast<JpegLibraryType::JDIMENSION>(input.width / 2);
            cinfo->image_height     = static_cast<JpegLibraryType::JDIMENSION>(input.height / 2);
            cinfo->input_components = RgbColorComponentCount;
            cinfo->in_color_space   = JpegLibraryType::J_COLOR_SPACE::JCS_RGB;

            /* Set defaults/color space. */
            jpeg_set_defaults(cinfo);
            jpeg_set_colorspace(cinfo, JpegLibraryType::J_COLOR_SPACE::JCS_YCbCr);

            /* Configure sampling. */
            /* libjpeg-turbo doesn't actually have this field, as of now. */
            /* cinfo->do_fancy_downsampling = false; */
            cinfo->comp_info[0].h_samp_factor = 2;
            cinfo->comp_info[0].v_samp_factor = 1;
            cinfo->comp_info[1].h_samp_factor = 1;
            cinfo->comp_info[1].v_samp_factor = 1;
            cinfo->comp_info[2].h_samp_factor = 1;
            cinfo->comp_info[2].v_samp_factor = 1;

            /* Set the quality. */
            jpeg_set_quality(cinfo, quality, true);

            /* Configure remaining parameters. */
            cinfo->dct_method        = JpegLibraryType::J_DCT_METHOD::JDCT_ISLOW;
            cinfo->optimize_coding   = false;
            cinfo->write_JFIF_header = true;
        }

    }

    Result SoftwareJpegShrinker::ShrinkRgba8(SoftwareJpegShrinkerOutput &output, const SoftwareJpegShrinkerInput &input, int quality, void *work, size_t work_size) {
        CAPSRV_ABORT_UNLESS(util::IsAligned(input.width,  ImageSizeHorizonalUnit));
        CAPSRV_ABORT_UNLESS(util::IsAligned(input.height, ImageSizeVerticalUnitDecode));

        const u32 shrunk_width  = input.width / 2;
        const u32 shrunk_height = input.height / 2;
        CAPSRV_ABORT_UNLESS(util::IsAligned(shrunk_width, ImageSizeHorizonalUnit));

        CAPSRV_ABORT_UNLESS(output.dst != nullptr);

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

        /* Create our compression structures. */
        JpegLibraryType::jpeg_decompress_struct dcinfo = {};
        JpegLibraryType::jpeg_compress_struct   ecinfo = {};

        /* Here nintendo creates a work buffer structure containing work_start + work_size. */
        /* This seems to be a custom patch for/to libjpeg-turbo. */
        /* It would be desirable for us to mimic this, because it gives Nintendo strong */
        /* fixed memory usage guarantees. */
        /* TODO: Determine if it is feasible for us to recreate this ourselves, */
        /* Either by adding support to the devkitPro libjpeg-turbo portlib or otherwise. */
        AMS_UNUSED(work_end);

        /* Create our error managers. */
        JpegErrorHandler jerr_dc = { .result     = ResultSuccess(), };
        JpegErrorHandler jerr_ec = { .result     = ResultSuccess(), };
        jerr_dc.error_exit = JpegErrorHandler::HandleError,
        jerr_ec.error_exit = JpegErrorHandler::HandleError,

        /* Link our error managers to our compression structures. */
        dcinfo.err = jpeg_std_error(std::addressof(jerr_dc));
        ecinfo.err = jpeg_std_error(std::addressof(jerr_ec));

        /* Use setjmp, so that on error our handler will longjmp to return an error result. */
        if (setjmp(jerr_ec.jmp_buf) == 0) {
            if (setjmp(jerr_dc.jmp_buf) == 0) {
                /* Create our decompressor. */
                jpeg_create_decompress(std::addressof(dcinfo));
                ON_SCOPE_EXIT { jpeg_destroy_decompress(std::addressof(dcinfo)); };

                /* Setup our memory reader, ensure the header is correct. */
                jpeg_mem_src(std::addressof(dcinfo), const_cast<unsigned char *>(static_cast<const unsigned char *>(input.jpeg)), input.jpeg_size);
                R_UNLESS(jpeg_read_header(std::addressof(dcinfo), true) == JPEG_HEADER_OK,  capsrv::ResultAlbumInvalidFileData());

                /* Ensure width and height are correct. */
                R_UNLESS(dcinfo.image_width  == input.width,  capsrv::ResultAlbumInvalidFileData());
                R_UNLESS(dcinfo.image_height == input.height, capsrv::ResultAlbumInvalidFileData());

                /* Set output parameters. */
                dcinfo.out_color_space       = JpegLibraryType::J_COLOR_SPACE::JCS_RGB;
                dcinfo.dct_method            = JpegLibraryType::J_DCT_METHOD::JDCT_ISLOW;
                dcinfo.do_fancy_upsampling   = input.fancy_upsampling;
                dcinfo.do_block_smoothing    = input.block_smoothing;
                dcinfo.scale_num             = 1;
                dcinfo.scale_denom           = 2;

                /* Start decompression. */
                R_UNLESS(jpeg_start_decompress(std::addressof(dcinfo)) == TRUE, capsrv::ResultAlbumInvalidFileData());

                /* Check the parameters. */
                CAPSRV_ASSERT(dcinfo.output_width         == shrunk_width);
                CAPSRV_ASSERT(dcinfo.output_height        == shrunk_height);
                CAPSRV_ASSERT(dcinfo.out_color_components == RgbColorComponentCount);
                CAPSRV_ASSERT(dcinfo.output_components    == RgbColorComponentCount);

                /* Create our compressor. */
                jpeg_create_compress(std::addressof(ecinfo));
                ON_SCOPE_EXIT { jpeg_destroy_compress(std::addressof(ecinfo)); };

                /* Setup our memory writer. */
                unsigned long out_size = static_cast<unsigned long>(output.dst_size);
                jpeg_mem_dest(std::addressof(ecinfo), reinterpret_cast<unsigned char **>(std::addressof(output.dst)), std::addressof(out_size));

                /* Setup the encoding parameters. */
                SetupEncodingParameter(std::addressof(ecinfo), input, quality);

                /* Start compression. */
                jpeg_start_compress(std::addressof(ecinfo), true);

                /* Parse the scanlines. */
                {
                    /* Create our linebuffer structure. */
                    JpegLibraryType::JSAMPROW linebuffers[ImageSizeVerticalUnitEncode] = {};
                    for (int i = 0; i < ImageSizeVerticalUnitEncode; i++) {
                        linebuffers[i] = rgb_buffer + rgb_buffer_stride * i;
                    }

                    /* While we still have scanlines, parse! */
                    while (dcinfo.output_scanline < shrunk_height) {
                        /* Determine remaining scanlines. */
                        const auto remaining_scanlines = shrunk_height - dcinfo.output_scanline;
                        const auto cur_max_scanlines   = std::min<s32>(remaining_scanlines, ImageSizeVerticalUnitEncode);

                        /* If we have scanlines to decode, try to do so. */
                        auto writable_scanlines = 0;
                        while (writable_scanlines < cur_max_scanlines) {
                            const auto decoded = jpeg_read_scanlines(std::addressof(dcinfo), linebuffers + writable_scanlines, ImageSizeVerticalUnitDecode);
                            CAPSRV_ASSERT(decoded <= ImageSizeVerticalUnitDecode / 2);

                            writable_scanlines += decoded;
                        }

                        /* If we have scanlines to write, try to do so. */
                        jpeg_write_scanlines(std::addressof(ecinfo), linebuffers, writable_scanlines);
                    }
                }

                /* Finish the decompression. */
                R_UNLESS(jpeg_finish_decompress(std::addressof(dcinfo)) == TRUE, capsrv::ResultAlbumInvalidFileData());

                /* Finish the compression. */
                jpeg_finish_compress(std::addressof(ecinfo));

                /* Set the output size. */
                *output.out_size = out_size;
            } else {
                /* Some unknown error was caught by our handler. */
                R_THROW(capsrv::ResultAlbumInvalidFileData());
            }
        } else {
            /* Return the encoding result. */
            R_THROW(jerr_ec.result);
        }

        /* Write the size we decoded to output. */
        *output.out_width = static_cast<s32>(dcinfo.output_width);
        *output.out_width = static_cast<s32>(dcinfo.output_height);

        R_SUCCEED();
    }

}
