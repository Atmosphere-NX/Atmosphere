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
#define ZSTD_STATIC_LINKING_ONLY

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define ZSTDLIB_VISIBLE   static
#define ZSTDLIB_HIDDEN    static
#include "zstd.h"
#include "zstd.inc"
#pragma GCC diagnostic pop

namespace ams::util {

    size_t CompressZstd(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Basic size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());
        
        /* Additionally, we must check the compression boundary. */ 
        auto bound = ZSTD_compressBound(src_size);
        AMS_ABORT_UNLESS(!ZSTD_isError(bound));
        AMS_ABORT_UNLESS(dst_size >= bound);

        /* Use Zstd default level. */
        int compressionLevel = 3;
        
        /* This is just a wrapper around Zstd. */
        return ZSTD_compress(dst, dst_size, src, src_size, compressionLevel);
    }
    
    size_t DecompressZstd(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Basic size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());
        
        /* Additionally, we must check the decompression boundary. */ 
        auto bound = ZSTD_decompressBound(src, src_size);
        AMS_ABORT_UNLESS(!ZSTD_isError(bound));
        AMS_ABORT_UNLESS(dst_size >= bound);
        
        /* This is just a wrapper around Zstd. */
        return ZSTD_decompress(dst, dst_size, src, src_size);
    }

}