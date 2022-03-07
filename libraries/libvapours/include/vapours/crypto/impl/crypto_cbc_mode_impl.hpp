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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>
#include <vapours/crypto/crypto_aes_encryptor.hpp>

namespace ams::crypto::impl {

    template<typename BlockCipher>
    class CbcModeImpl {
        NON_COPYABLE(CbcModeImpl);
        NON_MOVEABLE(CbcModeImpl);
        public:
            static constexpr size_t KeySize   = BlockCipher::KeySize;
            static constexpr size_t BlockSize = BlockCipher::BlockSize;
            static constexpr size_t IvSize    = BlockCipher::BlockSize;
        private:
            enum State {
                State_None,
                State_Initialized,
            };
        private:
            const BlockCipher *m_block_cipher;
            u8 m_iv[IvSize];
            u8 m_buffer[BlockSize];
            size_t m_buffered_bytes;
            State m_state;
        public:
            CbcModeImpl() : m_state(State_None) { /* ... */ }

            ~CbcModeImpl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize(const BlockCipher *block_cipher, const void *iv, size_t iv_size) {
                AMS_ASSERT(iv_size == IvSize);
                AMS_UNUSED(iv_size);

                m_block_cipher = block_cipher;
                std::memcpy(m_iv, iv, IvSize);
                m_buffered_bytes = 0;

                m_state = State_Initialized;
            }

            size_t GetBufferedDataSize() const {
                return m_buffered_bytes;
            }

            size_t UpdateEncryption(void *dst, size_t dst_size, const void *src, size_t src_size) {
                AMS_ASSERT(dst_size >= ((src_size + this->GetBufferedDataSize()) / BlockSize) * BlockSize);
                AMS_ASSERT(m_state == State_Initialized);

                return this->Update(dst, dst_size, src, src_size, [&] (u8 *d, u8 *i, const u8 *s, size_t n) ALWAYS_INLINE_LAMBDA {
                    this->EncryptBlocks(d, i, s, n);
                });
            }

            size_t UpdateDecryption(void *dst, size_t dst_size, const void *src, size_t src_size) {
                AMS_ASSERT(dst_size >= ((src_size + this->GetBufferedDataSize()) / BlockSize) * BlockSize);
                AMS_ASSERT(m_state == State_Initialized);

                return this->Update(dst, dst_size, src, src_size, [&] (u8 *d, u8 *i, const u8 *s, size_t n) ALWAYS_INLINE_LAMBDA {
                    this->DecryptBlocks(d, i, s, n);
                });
            }
        private:
            size_t Update(void *_dst, size_t dst_size, const void *_src, size_t src_size, auto ProcessBlocks) {
                AMS_UNUSED(dst_size);

                u8 *dst = static_cast<u8 *>(_dst);
                const u8 *src = static_cast<const u8 *>(_src);
                size_t remaining = src_size;
                size_t processed = 0;

                if (m_buffered_bytes > 0) {
                    const size_t copy_size = std::min(BlockSize - m_buffered_bytes, remaining);

                    std::memcpy(m_buffer + m_buffered_bytes, src, copy_size);
                    src                 += copy_size;
                    remaining           -= copy_size;
                    m_buffered_bytes    += copy_size;

                    if (m_buffered_bytes == BlockSize) {
                        ProcessBlocks(dst, m_iv, m_buffer, 1);
                        processed += BlockSize;
                        dst       += BlockSize;

                        m_buffered_bytes = 0;
                    }
                }

                if (remaining >= BlockSize) {
                    const size_t num_blocks = remaining / BlockSize;

                    ProcessBlocks(dst, m_iv, src, num_blocks);

                    const size_t processed_size = num_blocks * BlockSize;
                    dst       += processed_size;
                    src       += processed_size;
                    remaining -= processed_size;
                    processed += processed_size;
                }

                if (remaining > 0) {
                    std::memcpy(m_buffer, src, remaining);
                    m_buffered_bytes = remaining;
                }

                return processed;
            }

            void EncryptBlocks(u8 *dst, u8 *iv, const u8 *src, size_t num_blocks) {
                const u8 *cur_iv = iv;

                u8 block[BlockSize];
                while (num_blocks--) {
                    for (size_t i = 0; i < BlockSize; ++i) {
                        block[i] = src[i] ^ cur_iv[i];
                    }

                    m_block_cipher->EncryptBlock(dst, BlockSize, block, BlockSize);

                    cur_iv = dst;
                    src += BlockSize;
                    dst += BlockSize;
                }

                if (iv != cur_iv) {
                    std::memcpy(iv, cur_iv, BlockSize);
                }
            }

            void DecryptBlocks(u8 *dst, u8 *iv, const u8 *src, size_t num_blocks) {
                u8 next_iv[BlockSize];
                std::memcpy(next_iv, src + ((num_blocks - 1) * BlockSize), BlockSize);

                if (src == dst) {
                    src = src + ((num_blocks - 1) * BlockSize);
                    dst = dst + ((num_blocks - 1) * BlockSize);

                    const u8 *cur_iv = (num_blocks == 1) ? iv : src - BlockSize;

                    while (num_blocks-- > 1) {
                        m_block_cipher->DecryptBlock(dst, BlockSize, src, BlockSize);

                        for (size_t i = 0; i < BlockSize; ++i) {
                            dst[i] ^= cur_iv[i];
                        }

                        cur_iv -= BlockSize;
                        src    -= BlockSize;
                        dst    -= BlockSize;
                    }

                    m_block_cipher->DecryptBlock(dst, BlockSize, src, BlockSize);

                    for (size_t i = 0; i < BlockSize; ++i) {
                        dst[i] ^= iv[i];
                    }
                } else {
                    const u8 *cur_iv = iv;

                    while (num_blocks-- > 0) {
                        m_block_cipher->DecryptBlock(dst, BlockSize, src, BlockSize);

                        for (size_t i = 0; i < BlockSize; ++i) {
                            dst[i] ^= cur_iv[i];
                        }

                        cur_iv = src;
                        src += BlockSize;
                        dst += BlockSize;
                    }
                }

                std::memcpy(iv, next_iv, BlockSize);
            }
    };

    /* TODO: Optimized AES cbc impl specializations. */

}
