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
        /* Destination buffer must be behind source buffer. */
        if (dst > src) {
            std::memset(dst, 0, dst_size);
            return -1;
        }
        
        const char *src_end = static_cast<const char *>(src) + src_size;
        const char *dst_end = static_cast<const char *>(dst) + dst_size;
        
        if ((src_end != dst_end) || (src_work_size != 0x10000) || (dst_work_size != 0x10000)) {
            std::memset(dst, 0, dst_size);
            return -1;
        }
        
        if (src_size < 7) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        const uint32_t magic = *reinterpret_cast<const uint32_t *>(src);
        const uint8_t flg = reinterpret_cast<const uint8_t *>(src)[4];
        const uint8_t bd = reinterpret_cast<const uint8_t *>(src)[5];
        const uint8_t header_checksum = reinterpret_cast<const uint8_t *>(src)[6];
        
        /* Check for LZ4F_MAGICNUMBER, validate flags and block size. */
        if ((magic != 0x184D2204) || ((flg & 1) != 0) || (flg != 0x60) || (bd != 0x40)) {
            std::memset(dst, 0, dst_size);
            return -2;
        }
        
        /* TODO: Implement header checksum validation. This requires xxhash, do we want to have xxhash in util? */
        AMS_UNUSED(header_checksum);
        
        uint32_t src_pos = 7; /* Skip the header. */
        uint32_t dst_pos = 0;
        
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
            
            const char* src_data_block = reinterpret_cast<const char *>(src) + src_pos;
            char* dst_data_block = reinterpret_cast<char *>(dst) + dst_pos;
            
            const uint32_t block_header = *reinterpret_cast<const uint32_t *>(src_data_block);
            
            /* We've reached the end. */
            if (!block_header) {
                src_pos += sizeof(block_header);
                break;
            }
            
            const uint32_t block_size = block_header & 0x7FFFFFFF;
            
            /* Block size is invalid. */
            if (block_size > 0x10000) {
                std::memset(dst, 0, dst_size);
                return -3;
            }
            
            /* Advance the data block. */
            src_data_block += sizeof(block_header);
            
            /* Block size is invalid. */
            if ((dst_end - src_data_block) < block_size) {
                std::memset(dst, 0, dst_size);
                return -4;
            }
            
            /* Copy the data block to the source work buffer. */
            std::memcpy(static_cast<char *>(src_work), src_data_block, block_size);
            
            if ((block_header & 0x80000000) != 0) {
                /* Block size is invalid. */
                if ((dst_end - dst_data_block) < block_size) {
                    std::memset(dst, 0, dst_size);
                    return -4;
                }
                
                dst_pos += block_size;
                src_pos += sizeof(block_header) + block_size;
                
                /* Destination is overrunning source. */
                if (reinterpret_cast<char *>(dst) + dst_pos > reinterpret_cast<const char *>(src) + src_pos) {
                    std::memset(dst, 0, dst_size);
                    return -5;
                }
                
                /* This is a raw block, copy it. */
                std::memmove(dst_data_block, src_data_block, block_size); 
            } else {
                /* This is a compressed block, decompress it. */
                int decompressed_size = LZ4_decompress_safe(reinterpret_cast<const char *>(src_work), reinterpret_cast<char *>(dst_work), static_cast<int>(block_size), static_cast<int>(dst_work_size));
                
                /* Decompressed size is invalid. */
                if (decompressed_size < 1) {
                    std::memset(dst, 0, dst_size);
                    return -3;
                }
                
                /* Decompressed size is invalid. */
                if ((dst_end - dst_data_block) < decompressed_size) {
                    std::memset(dst, 0, dst_size);
                    return -4;
                }
                
                dst_pos += decompressed_size;
                src_pos += sizeof(block_header) + block_size;
                
                /* Destination is overrunning source. */
                if (reinterpret_cast<char *>(dst) + dst_pos > reinterpret_cast<const char *>(src) + src_pos) {
                    std::memset(dst, 0, dst_size);
                    return -5;
                }
                
                /* Copy the decompressed data back. */
                std::memcpy(dst_data_block, static_cast<char *>(dst_work), decompressed_size);
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