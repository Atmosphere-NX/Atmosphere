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
#include <vapours/crypto/impl/crypto_hash_function.hpp>
#include <vapours/crypto/impl/crypto_cbc_mac_impl.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>

namespace ams::crypto::impl {

    template<typename BlockCipher>
    class CmacImpl {
        NON_COPYABLE(CmacImpl);
        NON_MOVEABLE(CmacImpl);
        public:
            static constexpr size_t BlockSize = BlockCipher::BlockSize;
            static constexpr size_t MacSize   = BlockSize;
            static_assert(BlockSize == 0x10); /* TODO: Should this be supported? */
        private:
            enum State {
                State_None        = 0,
                State_Initialized = 1,
                State_Done        = 2,
            };
        private:
            CbcMacImpl m_cbc_mac_impl;
            u8 m_sub_key[BlockSize];
            State m_state;
        public:
            CmacImpl() : m_state(State_None) { /* ... */ }
            ~CmacImpl() {
                /* Clear everything. */
                ClearMemory(this, sizeof(*this));
            }

            void Initialize(const BlockCipher *cipher);
            void Update(const void *data, size_t data_size);
            void GetMac(void *dst, size_t dst_size);
        private:
            static void MultiplyOneOverGF128(u8 *data) {
                /* Determine the carry bit. */
                const u8 carry = data[0] & 0x80;

                /* Shift all bytes by one bit. */
                for (size_t i = 0; i < BlockSize - 1; ++i) {
                    data[i] = (data[i] << 1) | (data[i + 1] >> 7);
                }
                data[BlockSize - 1] <<= 1;

                /* Adjust based on carry. */
                if (carry) {
                    data[BlockSize - 1] ^= 0x87;
                }
            }
    };

    template<typename BlockCipher>
    inline void CmacImpl<BlockCipher>::Initialize(const BlockCipher *cipher) {
        /* Clear the key storage. */
        std::memset(m_sub_key, 0, sizeof(m_sub_key));

        /* Set the key storage. */
        cipher->EncryptBlock(m_sub_key, BlockSize, m_sub_key, BlockSize);
        MultiplyOneOverGF128(m_sub_key);

        /* Initialize the cbc-mac impl. */
        m_cbc_mac_impl.Initialize(cipher);

        /* Mark initialized. */
        m_state = State_Initialized;
    }

    template<typename BlockCipher>
    inline void CmacImpl<BlockCipher>::Update(const void *data, size_t data_size) {
        AMS_ASSERT(m_state == State_Initialized);

        m_cbc_mac_impl.template Update<BlockCipher>(data, data_size);
    }

    template<typename BlockCipher>
    inline void CmacImpl<BlockCipher>::GetMac(void *dst, size_t dst_size) {
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Done);
        AMS_ASSERT(dst_size >= MacSize);
        AMS_UNUSED(dst_size);

        /* If we're not already finalized, get the final mac. */
        if (m_state == State_Initialized) {
            /* Process padding as needed. */
            if (m_cbc_mac_impl.GetBufferedDataSize() != BlockSize) {
                /* Determine the remaining size. */
                const size_t remaining = BlockSize - m_cbc_mac_impl.GetBufferedDataSize();

                /* Update with padding. */
                static constexpr u8 s_padding[BlockSize] = { 0x80, /* ... */ };
                m_cbc_mac_impl.template Update<BlockCipher>(s_padding, remaining);

                /* Update our subkey. */
                MultiplyOneOverGF128(m_sub_key);
            }

            /* Mask the subkey. */
            m_cbc_mac_impl.MaskBufferedData(m_sub_key, BlockSize);

            /* Set our state as done. */
            m_state = State_Done;
        }

        /* Get the mac. */
        m_cbc_mac_impl.GetMac(dst, dst_size);
    }

}
