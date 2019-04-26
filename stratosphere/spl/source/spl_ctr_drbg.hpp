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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "spl_types.hpp"

/* Nintendo implements CTR_DRBG for their csrng. We will do the same. */
class CtrDrbg {
    public:
        static constexpr size_t MaxRequestSize = 0x10000;
        static constexpr size_t ReseedInterval = 0x7FFFFFF0;
        static constexpr size_t BlockSize = AES_BLOCK_SIZE;
        static constexpr size_t SeedSize = 2 * AES_BLOCK_SIZE;
    private:
        Aes128Context aes_ctx;
        u8 counter[BlockSize];
        u8 key[BlockSize];
        u8 work[2][SeedSize];
        u32 reseed_counter;
    private:
        static void Xor(void *dst, const void *src, size_t size) {
            const u8 *src_u8 = reinterpret_cast<const u8 *>(src);
            u8 *dst_u8 = reinterpret_cast<u8 *>(dst);

            for (size_t i = 0; i < size; i++) {
                dst_u8[i] ^= src_u8[i];
            }
        }

        static void IncrementCounter(void *ctr) {
            u64 *ctr_64 = reinterpret_cast<u64 *>(ctr);

            ctr_64[1] = __builtin_bswap64(__builtin_bswap64(ctr_64[1]) + 1);
            if (!ctr_64[1]) {
                ctr_64[0] = __builtin_bswap64(__builtin_bswap64(ctr_64[0]) + 1);
            }
        }
    private:
        void Update(const void *data);
    public:
        void Initialize(const void *seed);
        void Reseed(const void *seed);
        bool GenerateRandomBytes(void *out, size_t size);
};