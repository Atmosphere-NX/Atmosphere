/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include <jpeglib.h>
#include <jerror.h>

namespace ams::capsrv::server::jpeg {

    class JpegLibraryType {
        public:
            using jpeg_common_struct     = ::jpeg_common_struct;
            using jpeg_compress_struct   = ::jpeg_compress_struct;
            using jpeg_decompress_struct = ::jpeg_decompress_struct;
            using jpeg_error_mgr         = ::jpeg_error_mgr;
            using jpeg_destination_mgr   = ::jpeg_destination_mgr;

            using j_common_ptr           = ::j_common_ptr;
            using j_compress_ptr         = ::j_compress_ptr;

            using boolean                = ::boolean;
            using JOCTET                 = ::JOCTET;
            using JDIMENSION             = ::JDIMENSION;

            using JSAMPARRAY             = ::JSAMPARRAY;
            using JSAMPROW               = ::JSAMPROW;

            using J_COLOR_SPACE          = ::J_COLOR_SPACE;
            using J_DCT_METHOD           = ::J_DCT_METHOD;
            using J_MESSAGE_CODE         = ::J_MESSAGE_CODE;
    };

}
