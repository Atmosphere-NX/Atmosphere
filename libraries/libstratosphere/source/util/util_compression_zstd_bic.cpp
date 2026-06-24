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
#define ZSTD_ZBIC_SUPPORT 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#define ZSTDLIB_VISIBLE   static
#define ZSTDLIB_HIDDEN    static
#include "zstd.h"
#include "zstd.inc"
#pragma GCC diagnostic pop

namespace ams::util {
    static_assert(sizeof(ZSTD_DCtx) <= DecompressZstdWithBicWorkBufferSizeDefault);

    size_t CompressZstdWithBic(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Basic size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());
        
        /* Additionally, we must check the compression boundary. */ 
        auto bound = ZSTD_compressBound(src_size);
        AMS_ABORT_UNLESS(!ZSTD_isError(bound));
        AMS_ABORT_UNLESS(dst_size >= bound);

        /* Use Zstd default level. */
        int compressionLevel = 3;
        
        /* This is just a wrapper around Zstd with BIC support enabled. */
        return ZSTD_compress(dst, dst_size, src, src_size, compressionLevel);
    }
    
    size_t DecompressZstdWithBic(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Basic size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());
        
        /* Additionally, we must check the decompression boundary. */ 
        auto bound = ZSTD_decompressBound(src, src_size);
        AMS_ABORT_UNLESS(!ZSTD_isError(bound));
        AMS_ABORT_UNLESS(dst_size >= bound);
        
        /* This is just a wrapper around Zstd with BIC support enabled. */
        return ZSTD_decompress(dst, dst_size, src, src_size);
    }
    
    bool DecompressZstdWithBic(void* workspace, size_t workspace_size, void *dst, size_t dst_size, size_t expected_dec_size, const void *src, size_t src_size) {
        /* Check decompression margin. */
        auto margin = ZSTD_decompressionMargin(src, src_size);
        if (ZSTD_isError(margin)) {
            auto error_code = ZSTD_getErrorCode(margin);
            auto error_string = ZSTD_getErrorString(error_code);
            
            AMS_LOG("[util] Can't determine decompression margin: %u (%s)\n", error_code, error_string);
            
            /* TODO: Unused variables in non-debug build. */
            AMS_UNUSED(error_code, error_string);
            
            return false;
        }
        
        /* Don't overflow from margin. */
        if (!util::CanAddWithoutOverflow(margin, expected_dec_size)) {
            return false;
        }
        
        /* Make sure we fit in the destination buffer. */
        if (margin + expected_dec_size > dst_size) {
            AMS_LOG("[util] Not enough space for decompression %lu + %lu > %lu\n", margin, expected_dec_size, dst_size);
            return false;
        }
        
        /* This is a runtime assert in Loader code. Note that Nintendo does == comparison here. */
        AMS_ABORT_UNLESS(ZSTD_estimateDCtxSize() <= workspace_size);
        
        /* Decompress using a static decompression context. */
        auto dctx = ZSTD_initStaticDCtx(workspace, workspace_size);
        size_t dec_size = ZSTD_decompressDCtx(dctx, dst, dst_size, src, src_size);

        if (ZSTD_isError(dec_size)) {
            auto error_code = ZSTD_getErrorCode(margin);
            auto error_string = ZSTD_getErrorString(error_code);
            
            AMS_LOG("[util] Decompression failed: %u (%s)\n", error_code, error_string);
            
            /* TODO: Unused variables in non-debug build. */
            AMS_UNUSED(error_code, error_string);
            
            return false;
        }
        
        /* Make sure we match the expected size. */
        if (dec_size != expected_dec_size) {
            return false;
        }
        
        return true;
    }

}