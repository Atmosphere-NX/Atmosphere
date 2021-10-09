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
    class CtrModeImpl {
        NON_COPYABLE(CtrModeImpl);
        NON_MOVEABLE(CtrModeImpl);
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
            u8 m_counter[IvSize];
            u8 m_encrypted_counter[BlockSize];
            size_t m_buffer_offset;
            State m_state;
        public:
            CtrModeImpl() : m_state(State_None) { /* ... */ }

            ~CtrModeImpl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize(const BlockCipher *block_cipher, const void *iv, size_t iv_size) {
                this->Initialize(block_cipher, iv, iv_size, 0);
            }

            void Initialize(const BlockCipher *block_cipher, const void *iv, size_t iv_size, s64 offset) {
                AMS_ASSERT(iv_size == IvSize);
                AMS_ASSERT(offset  >= 0);

                m_block_cipher = block_cipher;
                m_state        = State_Initialized;

                this->SwitchMessage(iv, iv_size);

                if (offset >= 0) {
                    u64 ctr_offset = offset / BlockSize;
                    if (ctr_offset > 0) {
                        this->IncrementCounter(ctr_offset);
                    }

                    if (size_t remaining = static_cast<size_t>(offset % BlockSize); remaining != 0) {
                        m_block_cipher->EncryptBlock(m_encrypted_counter, sizeof(m_encrypted_counter), m_counter, sizeof(m_counter));
                        this->IncrementCounter();

                        m_buffer_offset = remaining;
                    }
                }
            }

            void SwitchMessage(const void *iv, size_t iv_size) {
                AMS_ASSERT(m_state == State_Initialized);
                AMS_ASSERT(iv_size     == IvSize);

                std::memcpy(m_counter, iv, iv_size);
                m_buffer_offset = 0;
            }

            void IncrementCounter() {
                for (s32 i = IvSize - 1; i >= 0; --i) {
                    if (++m_counter[i] != 0) {
                        break;
                    }
                }
            }

            size_t Update(void *_dst, size_t dst_size, const void *_src, size_t src_size) {
                AMS_ASSERT(m_state == State_Initialized);
                AMS_ASSERT(dst_size >= src_size);
                AMS_UNUSED(dst_size);

                u8 *dst = static_cast<u8 *>(_dst);
                const u8 *src = static_cast<const u8 *>(_src);
                size_t remaining = src_size;

                if (m_buffer_offset > 0) {
                    const size_t xor_size = std::min(BlockSize - m_buffer_offset, remaining);

                    const u8 *ctr = m_encrypted_counter + m_buffer_offset;
                    for (size_t i = 0; i < xor_size; i++) {
                        dst[i] = src[i] ^ ctr[i];
                    }

                    src                 += xor_size;
                    dst                 += xor_size;
                    remaining           -= xor_size;
                    m_buffer_offset += xor_size;

                    if (m_buffer_offset == BlockSize) {
                        m_buffer_offset = 0;
                    }
                }

                if (remaining >= BlockSize) {
                    const size_t num_blocks = remaining / BlockSize;

                    this->ProcessBlocks(dst, src, num_blocks);

                    const size_t processed_size = num_blocks * BlockSize;
                    dst       += processed_size;
                    src       += processed_size;
                    remaining -= processed_size;
                }

                if (remaining > 0) {
                    this->ProcessBlock(dst, src, remaining);
                    m_buffer_offset = remaining;
                }

                return src_size;
            }
        private:
            void IncrementCounter(u64 count) {
                u64 _block[IvSize / sizeof(u64)] = {};
                util::StoreBigEndian(std::addressof(_block[(IvSize / sizeof(u64)) - 1]), count);

                u16 acc = 0;
                const u8 *block = reinterpret_cast<const u8 *>(_block);
                for (s32 i = IvSize - 1; i >= 0; --i) {
                    acc += (m_counter[i] + block[i]);
                    m_counter[i] = acc & 0xFF;
                    acc >>= 8;
                }
            }

            void ProcessBlock(u8 *dst, const u8 *src, size_t src_size) {
                m_block_cipher->EncryptBlock(m_encrypted_counter, BlockSize, m_counter, IvSize);
                this->IncrementCounter();

                for (size_t i = 0; i < src_size; i++) {
                    dst[i] = src[i] ^ m_encrypted_counter[i];
                }
            }

            void ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks);
    };

    template<typename BlockCipher>
    inline void CtrModeImpl<BlockCipher>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
        while (num_blocks--) {
            this->ProcessBlock(dst, src, BlockSize);
            dst += BlockSize;
            src += BlockSize;
        }
    }

    template<> void CtrModeImpl<AesEncryptor128>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks);
    template<> void CtrModeImpl<AesEncryptor192>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks);
    template<> void CtrModeImpl<AesEncryptor256>::ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks);

}
