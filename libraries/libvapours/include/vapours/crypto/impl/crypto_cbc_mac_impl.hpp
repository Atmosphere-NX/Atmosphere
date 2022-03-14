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

    class CbcMacImpl {
        NON_COPYABLE(CbcMacImpl);
        NON_MOVEABLE(CbcMacImpl);
        public:
            static constexpr size_t BlockSize = 0x10;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_Done,
            };
        private:
            u8 m_mac[BlockSize];
            u8 m_buffer[BlockSize];
            size_t m_buffered_bytes;
            const void *m_cipher_context;
            void (*m_cipher_function)(void *dst, const void *src, const void *ctx);
            State m_state;
        public:
            CbcMacImpl() : m_buffered_bytes(0), m_state(State_None) { /* ... */ }

            ~CbcMacImpl() {
                ClearMemory(this, sizeof(*this));
            }

            template<typename BlockCipher>
            void Initialize(const BlockCipher *block_cipher) {
                static_assert(BlockCipher::BlockSize == BlockSize);

                /* Set our context. */
                m_cipher_context  = block_cipher;
                m_cipher_function = &EncryptBlockCallback<BlockCipher>;
                m_buffered_bytes  = 0;

                std::memset(m_mac, 0, sizeof(m_mac));

                m_state = State_Initialized;
            }

            template<typename BlockCipher>
            void Update(const void *data, size_t size) {
                this->UpdateGeneric(data, size);
            }

            template<typename BlockCipher>
            void ProcessBlocks(const void *data, size_t size) {
                this->ProcessBlocksGeneric(data, size);
            }

            size_t GetBlockSize() const {
                return BlockSize;
            }

            size_t GetBufferedDataSize() const {
                return m_buffered_bytes;
            }

            void UpdateGeneric(const void *data, size_t size);
            void ProcessBlocksGeneric(const void *data, size_t num_blocks);
            void ProcessPartialData(const void *data, size_t size);
            void ProcessRemainingData(const void *data, size_t size);

            void GetMac(void *mac, size_t mac_size);
            void MaskBufferedData(const void *data, size_t size);
        private:
            void ProcessBlock(const void *data);

            template<typename BlockCipher>
            static void EncryptBlockCallback(void *dst, const void *src, const void *cipher) {
                static_assert(BlockCipher::BlockSize == BlockSize);
                static_cast<const BlockCipher *>(cipher)->EncryptBlock(dst, BlockCipher::BlockSize, src, BlockCipher::BlockSize);
            }
    };

    template<>
    void CbcMacImpl::Update<AesEncryptor128>(const void *data, size_t size);

}
