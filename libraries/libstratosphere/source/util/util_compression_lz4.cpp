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
#include "lz4.h"

namespace ams::util {

    int CompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());

        /* This is just a thin wrapper around LZ4. */
        return LZ4_compress_default(reinterpret_cast<const char *>(src), reinterpret_cast<char *>(dst), static_cast<int>(src_size), static_cast<int>(dst_size));
    }

    int DecompressLZ4(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Size checks. */
        AMS_ABORT_UNLESS(dst_size <= std::numeric_limits<int>::max());
        AMS_ABORT_UNLESS(src_size <= std::numeric_limits<int>::max());

        /* This is just a thin wrapper around LZ4. */
        return LZ4_decompress_safe(reinterpret_cast<const char *>(src), reinterpret_cast<char *>(dst), static_cast<int>(src_size), static_cast<int>(dst_size));
    }
    
    /* TODO: This was added for ro in 23.0.0+, but it's unclear if this is even part of the util namespace. */
    /* It's similar to the official lz4frame code, but has additional constraints and uses static work buffers. */
    /* Evaluate if we should use lz4frame instead and implement matching frame compression. */
    int DecompressLZ4Frame(size_t* out_size, void *dst, size_t dst_size, const void *src, size_t src_size, void *src_work, size_t src_work_size, void *dst_work, size_t dst_work_size) {
        const u8 *src_buffer = reinterpret_cast<const u8 *>(src);
        u8 *dst_buffer = reinterpret_cast<u8 *>(dst);
        
        /* Destination buffer must be behind source buffer. */
        if (dst_buffer > src_buffer) {
            std::memset(dst, 0, dst_size);
            return -1;
        }
        
        /* Size checks. */
        if (((src_buffer + src_size) != (dst_buffer + dst_size)) || (src_work_size != 0x10000) || (dst_work_size != 0x10000)) {
            std::memset(dst, 0, dst_size);
            return -1;
        }
        
        /* Source size must at least include one frame header. */
        if (src_size < 7) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        const u32 magic = *reinterpret_cast<const u32 *>(src_buffer);
        const u8 flg = src_buffer[4];
        const u8 bd = src_buffer[5];
        const u8 header_checksum = src_buffer[6];
        
        /* Check for LZ4F_MAGICNUMBER, validate flags and block size. */
        if ((magic != 0x184D2204) || ((flg & 1) != 0) || (flg != 0x60) || (bd != 0x40)) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        /* TODO: Implement header checksum validation. This requires xxhash, do we want to have xxhash in util? */
        AMS_UNUSED(header_checksum);
        
        u32 src_pos = 7; /* Skip the header. */
        u32 dst_pos = 0;
        
        /* Destination size is too small. */
        if (dst_size < src_pos) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        while (true) {
            /* Not enough space left. */
            if ((dst_size - src_pos) < 4) {
                std::memset(dst, 0, dst_size);
                return -4;
            }
            
            src_buffer += src_pos;
            dst_buffer += dst_pos;
            
            const u32 block_header = *reinterpret_cast<const u32 *>(src_buffer);
            
            /* We've reached the end. */
            if (!block_header) {
                break;
            }
            
            const u32 block_size = block_header & 0x7FFFFFFF;
            
            /* Block size is invalid. */
            if (block_size > 0x10000) {
                std::memset(dst, 0, dst_size);
                return -3;
            }
            
            /* Advance the data block. */
            src_buffer += sizeof(block_header);
            
            /* Block size is invalid. */
            if (((dst_buffer + dst_size) - src_buffer) < block_size) {
                std::memset(dst, 0, dst_size);
                return -4;
            }
            
            /* Copy the data block to the source work buffer. */
            std::memcpy(static_cast<u8 *>(src_work), src_buffer, block_size);
            
            if ((block_header & 0x80000000) != 0) {
                /* Block size is invalid. */
                if (dst_size < block_size) {
                    std::memset(dst, 0, dst_size);
                    return -4;
                }
                
                dst_pos += block_size;
                src_pos += block_size;
                
                /* Destination is overrunning source. */
                if (dst_pos > src_pos) {
                    std::memset(dst, 0, dst_size);
                    return -5;
                }
                
                /* This is a raw block, copy it. */
                std::memmove(dst_buffer, src_buffer, block_size); 
            } else {
                /* This is a compressed block, decompress it. */
                int decompressed_size = LZ4_decompress_safe(reinterpret_cast<const char *>(src_work), reinterpret_cast<char *>(dst_work), static_cast<int>(block_size), 0x10000);
                
                /* Decompressed size is invalid. */
                if (decompressed_size < 1) {
                    std::memset(dst, 0, dst_size);
                    return -3;
                }
                
                /* Decompressed size is invalid. */
                if (dst_size < static_cast<size_t>(decompressed_size)) {
                    std::memset(dst, 0, dst_size);
                    return -4;
                }
                
                dst_pos += decompressed_size;
                src_pos += block_size;
                
                /* Destination is overrunning source. */
                if (dst_pos > src_pos) {
                    std::memset(dst, 0, dst_size);
                    return -5;
                }
                
                /* Copy the decompressed data back. */
                std::memcpy(dst_buffer, static_cast<u8 *>(dst_work), decompressed_size);
            }
            
            /* We've exceeded the destination's size. */
            if (src_pos >= dst_size) {
                std::memset(dst, 0, dst_size);
                return -2;
            }   
        }
        
        /* We didn't consume all data. */
        if (src_pos != src_size) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        *out_size = dst_pos;
        return 0;
    }
}