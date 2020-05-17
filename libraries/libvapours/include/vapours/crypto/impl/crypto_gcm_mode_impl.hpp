/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>
#include <vapours/crypto/crypto_aes_encryptor.hpp>

namespace ams::crypto::impl {

    template<typename BlockCipher>
    class GcmModeImpl {
        NON_COPYABLE(GcmModeImpl);
        NON_MOVEABLE(GcmModeImpl);
        public:
            static constexpr size_t KeySize   = BlockCipher::KeySize;
            static constexpr size_t BlockSize = BlockCipher::BlockSize;
            static constexpr size_t MacSize   = BlockCipher::BlockSize;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_ProcessingAad,
                State_Encrypting,
                State_Decrypting,
                State_Done,
            };

            struct Block128 {
                u64 hi;
                u64 lo;

                ALWAYS_INLINE void Clear() {
                    this->hi = 0;
                    this->lo = 0;
                }
            };
            static_assert(util::is_pod<Block128>::value);
            static_assert(sizeof(Block128) == 0x10);

            union Block {
                Block128 block_128;
                u32      block_32[4];
                u8       block_8[16];
            };
            static_assert(util::is_pod<Block>::value);
            static_assert(sizeof(Block) == 0x10);

            using CipherFunction = void (*)(void *dst_block, const void *src_block, const void *ctx);
        private:
            State state;
            const BlockCipher *block_cipher;
            CipherFunction cipher_func;
            u8 pad[sizeof(u64)];
            Block block_x;
            Block block_y;
            Block block_ek;
            Block block_ek0;
            Block block_tmp;
            size_t aad_size;
            size_t msg_size;
            u32    aad_remaining;
            u32    msg_remaining;
            u32    counter;
            Block  h_mult_blocks[16];
        public:
            GcmModeImpl() : state(State_None) { /* ... */ }

            ~GcmModeImpl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize(const BlockCipher *block_cipher);

            void Reset(const void *iv, size_t iv_size);

            void   UpdateAad(const void *aad, size_t aad_size);
            size_t UpdateEncrypt(void *dst, size_t dst_size, const void *src, size_t src_size);
            size_t UpdateDecrypt(void *dst, size_t dst_size, const void *src, size_t src_size);

            void GetMac(void *dst, size_t dst_size);
        private:
            static void ProcessBlock(void *dst_block, const void *src_block, const void *ctx) {
                static_cast<const BlockCipher *>(ctx)->EncryptBlock(dst_block, BlockSize, src_block, BlockSize);
            }

            void InitializeHashKey();
            void ComputeMac(bool encrypt);
    };

}
