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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/util.hpp>

#include "lz4.h"

namespace sts::util {

    /* Compression utilities. */
    int CompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Size checks. */
        if (dst_size > std::numeric_limits<int>::max() || src_size > std::numeric_limits<int>::max()) {
            std::abort();
        }

        /* This is just a thin wrapper around LZ4. */
        return LZ4_compress_default(reinterpret_cast<const char *>(src), reinterpret_cast<char *>(dst), static_cast<int>(src_size), static_cast<int>(dst_size));
    }

    /* Decompression utilities. */
    int DecompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Size checks. */
        if (dst_size > std::numeric_limits<int>::max() || src_size > std::numeric_limits<int>::max()) {
            std::abort();
        }

        /* This is just a thin wrapper around LZ4. */
        return LZ4_decompress_safe(reinterpret_cast<const char *>(src), reinterpret_cast<char *>(dst), static_cast<int>(src_size), static_cast<int>(dst_size));
    }

}