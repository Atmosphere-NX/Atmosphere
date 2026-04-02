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

#pragma once
#include <vapours.hpp>

namespace ams::util {

    /* Compression utilities. */
    int CompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size);
    size_t CompressZstd(void *dst, size_t dst_size, const void *src, size_t src_size);

    /* Decompression utilities. */
    int DecompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size);
    size_t DecompressZstd(void *dst, size_t dst_size, const void *src, size_t src_size);
    bool DecompressZstdForLoader(void* workspace, size_t workspace_size, void *dst, size_t dst_size, size_t expected_dec_size, const void *src, size_t src_size);

}