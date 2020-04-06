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

    class XtsModeImpl {
        NON_COPYABLE(XtsModeImpl);
        NON_MOVEABLE(XtsModeImpl);
        public:
            /* TODO: More generic support. */
            static constexpr size_t BlockSize = 16;
            static constexpr size_t IvSize    = 16;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_Processing,
                State_Done
            };
        private:
            u8 buffer[BlockSize];
            u8 tweak[BlockSize];
            u8 last_block[BlockSize];
            size_t num_buffered;
            const void *cipher_ctx;
            void (*cipher_func)(void *dst_block, const void *src_block, const void *cipher_ctx);
            State state;
        public:
            XtsModeImpl() : num_buffered(0), state(State_None) { /* ... */ }

            ~XtsModeImpl() {
                ClearMemory(this, sizeof(*this));
            }
        private:
            template<typename BlockCipher>
            static void EncryptBlockCallback(void *dst_block, const void *src_block, const void *cipher) {
                return static_cast<const BlockCipher *>(cipher)->EncryptBlock(dst_block, BlockCipher::BlockSize, src_block, BlockCipher::BlockSize);
            }

            template<typename BlockCipher>
            static void DecryptBlockCallback(void *dst_block, const void *src_block, const void *cipher) {
                return static_cast<const BlockCipher *>(cipher)->DecryptBlock(dst_block, BlockCipher::BlockSize, src_block, BlockCipher::BlockSize);
            }

            template<typename BlockCipher>
            void Initialize(const BlockCipher *cipher, const void *tweak, size_t tweak_size) {
                AMS_ASSERT(tweak_size == IvSize);

                cipher->EncryptBlock(this->tweak, IvSize, tweak, IvSize);

                this->num_buffered = 0;
                this->state = State_Initialized;
            }

            void ProcessBlock(u8 *dst, const u8 *src);
        public:
            template<typename BlockCipher1, typename BlockCipher2>
            void InitializeEncryption(const BlockCipher1 *cipher1, const BlockCipher2 *cipher2, const void *tweak, size_t tweak_size) {
                static_assert(BlockCipher1::BlockSize == BlockSize);
                static_assert(BlockCipher2::BlockSize == BlockSize);

                this->cipher_ctx  = cipher1;
                this->cipher_func = EncryptBlockCallback<BlockCipher1>;

                this->Initialize(cipher2, tweak, tweak_size);
            }

            template<typename BlockCipher1, typename BlockCipher2>
            void InitializeDecryption(const BlockCipher1 *cipher1, const BlockCipher2 *cipher2, const void *tweak, size_t tweak_size) {
                static_assert(BlockCipher1::BlockSize == BlockSize);
                static_assert(BlockCipher2::BlockSize == BlockSize);

                this->cipher_ctx  = cipher1;
                this->cipher_func = DecryptBlockCallback<BlockCipher1>;

                this->Initialize(cipher2, tweak, tweak_size);
            }

            template<typename BlockCipher>
            size_t Update(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return this->UpdateGeneric(dst, dst_size, src, src_size);
            }

            template<typename BlockCipher>
            size_t ProcessBlocks(u8 *dst, const u8 *src, size_t num_blocks) {
                return this->ProcessBlocksGeneric(dst, src, num_blocks);
            }

            size_t GetBufferedDataSize() const {
                return this->num_buffered;
            }

            constexpr size_t GetBlockSize() const {
                return BlockSize;
            }

            size_t FinalizeEncryption(void *dst, size_t dst_size);
            size_t FinalizeDecryption(void *dst, size_t dst_size);

            size_t UpdateGeneric(void *dst, size_t dst_size, const void *src, size_t src_size);
            size_t ProcessBlocksGeneric(u8 *dst, const u8 *src, size_t num_blocks);

            size_t ProcessPartialData(u8 *dst, const u8 *src, size_t size);
            size_t ProcessRemainingData(u8 *dst, const u8 *src, size_t size);
    };

    template<> size_t XtsModeImpl::Update<AesEncryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size);
    template<> size_t XtsModeImpl::Update<AesEncryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size);
    template<> size_t XtsModeImpl::Update<AesEncryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size);

    template<> size_t XtsModeImpl::Update<AesDecryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size);
    template<> size_t XtsModeImpl::Update<AesDecryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size);
    template<> size_t XtsModeImpl::Update<AesDecryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size);

}
