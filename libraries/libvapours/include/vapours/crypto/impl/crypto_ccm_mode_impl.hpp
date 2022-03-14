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
#include <vapours/crypto/impl/crypto_ctr_mode_impl.hpp>
#include <vapours/crypto/impl/crypto_cbc_mac_impl.hpp>

namespace ams::crypto::impl {

    template<typename BlockCipher>
    class CcmModeImpl {
        NON_COPYABLE(CcmModeImpl);
        NON_MOVEABLE(CcmModeImpl);
        public:
            static constexpr size_t KeySize   = BlockCipher::KeySize;
            static constexpr size_t BlockSize = BlockCipher::BlockSize;
        private:
            enum State {
                State_None,
                State_ProcessingAad,
                State_ProcessingData,
                State_DataInputDone,
                State_Done,
            };
        private:
            u8 m_mac[BlockSize];
            s64 m_given_data_size;
            s64 m_given_aad_size;
            size_t m_given_mac_size;
            s64 m_processed_data_size;
            s64 m_processed_aad_size;
            State m_state;
            CtrModeImpl<BlockCipher> m_ctr_mode_impl;
            CbcMacImpl m_cbc_mac_impl;
        public:
            CcmModeImpl() : m_state(State_None) { /* ... */ }

            ~CcmModeImpl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize(const BlockCipher *cipher, const void *nonce, size_t nonce_size, s64 aad_size, s64 data_size, size_t mac_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(7 <= nonce_size && nonce_size <= 13);
                AMS_ASSERT(4 <= mac_size && mac_size <= 16 && (mac_size % 2) == 0);
                AMS_ASSERT(aad_size >= 0);
                AMS_ASSERT(data_size >= 0);
                if (nonce_size == 7) {
                    AMS_ASSERT(data_size <= std::numeric_limits<s64>::max());
                } else {
                    AMS_ASSERT(data_size < (INT64_C(1) << ((15 - nonce_size) * 8)));
                }

                /* Set various size fields. */
                m_given_aad_size      = aad_size;
                m_given_data_size     = data_size;
                m_given_mac_size      = mac_size;
                m_processed_aad_size  = 0;
                m_processed_data_size = 0;

                /* Make the initial counter. */
                u8 tmp[BlockSize];
                MakeInitialCounter(tmp, nonce, nonce_size);

                /* Encrypt the block. */
                cipher->EncryptBlock(m_mac, BlockSize, tmp, BlockSize);

                /* Initialize our ctr mode impl. */
                m_ctr_mode_impl.Initialize(cipher, tmp, BlockSize);
                m_ctr_mode_impl.IncrementCounter();

                /* Make the header block. */
                MakeHeaderBlock(tmp, nonce, nonce_size, aad_size, data_size, mac_size);

                /* Initialize our cbc mac impl. */
                m_cbc_mac_impl.Initialize(cipher);
                m_cbc_mac_impl.template Update<BlockCipher>(tmp, BlockSize);

                /* Process aad size block. */
                if (aad_size > 0) {
                    this->ProcessEncodedAadSize();
                    m_state = State_ProcessingAad;
                } else {
                    m_state = State_ProcessingData;
                }
            }

            void UpdateAad(const void *aad, size_t aad_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_state == State_ProcessingAad);
                AMS_ASSERT(m_processed_aad_size + static_cast<s64>(aad_size) <= m_given_aad_size);

                /* Update on the aad. */
                m_cbc_mac_impl.template Update<BlockCipher>(aad, aad_size);
                m_processed_aad_size += aad_size;

                /* Check if we're done with aad. */
                if (m_processed_aad_size == m_given_aad_size) {
                    /* Pad the aad to block size. */
                    this->ProcessPadding();

                    /* Update our state. */
                    if (m_given_data_size > 0) {
                        m_state = State_ProcessingData;
                    } else {
                        m_state = State_DataInputDone;
                    }
                }
            }

            size_t UpdateEncryption(void *dst, size_t dst_size, const void *src, size_t src_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_state == State_ProcessingData);
                AMS_ASSERT(m_processed_data_size + static_cast<s64>(src_size) <= m_given_data_size);
                AMS_ASSERT(dst_size >= src_size);

                /* Update mac on decrypted data. */
                m_cbc_mac_impl.template Update<BlockCipher>(src, src_size);

                /* Encrypt. */
                const size_t processed = m_ctr_mode_impl.Update(dst, dst_size, src, src_size);
                m_processed_data_size += src_size;

                /* Check if we're done with data. */
                if (m_processed_data_size == m_given_data_size) {
                    /* Pad the data to block size. */
                    this->ProcessPadding();

                    m_state = State_DataInputDone;
                }

                return processed;
            }

            size_t UpdateDecryption(void *dst, size_t dst_size, const void *src, size_t src_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_state == State_ProcessingData);
                AMS_ASSERT(m_processed_data_size + static_cast<s64>(src_size) <= m_given_data_size);
                AMS_ASSERT(dst_size >= src_size);

                /* Decrypt. */
                const size_t processed = m_ctr_mode_impl.Update(dst, dst_size, src, src_size);
                m_processed_data_size += src_size;

                /* Update mac on decrypted data. */
                m_cbc_mac_impl.template Update<BlockCipher>(dst, dst_size);

                /* Check if we're done with data. */
                if (m_processed_data_size == m_given_data_size) {
                    /* Pad the data to block size. */
                    this->ProcessPadding();

                    m_state = State_DataInputDone;
                }

                return processed;
            }

            void GetMac(void *mac, size_t mac_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_state == State_DataInputDone || m_state == State_Done);
                AMS_ASSERT(mac_size >= m_given_mac_size);
                AMS_UNUSED(mac_size);

                /* Generate the mac, if we haven't already. */
                if (m_state == State_DataInputDone) {
                    this->GenerateMac();
                    m_state = State_Done;
                }

                /* Copy out the mac. */
                std::memcpy(mac, m_mac, m_given_mac_size);
            }
        private:
            void MakeInitialCounter(void *dst, const void *nonce, size_t nonce_size) {
                /* Clear the counter. */
                u8 *ctr = static_cast<u8 *>(dst);
                std::memset(ctr, 0, BlockSize);

                /* Set the nonce. */
                ctr[0] = (((BlockSize - 1 - nonce_size) & 0xFF) - 1) & 0x07;
                std::memcpy(ctr + 1, nonce, nonce_size);
            }

            void MakeHeaderBlock(void *dst, const void *nonce, size_t nonce_size, s64 aad_size, s64 data_size, size_t mac_size) {
                /* Clear the block. */
                u8 *hdr = static_cast<u8 *>(dst);
                std::memset(hdr, 0, BlockSize);

                /* Encode the flags. */
                hdr[0] = (((BlockSize - 1 - nonce_size) & 0xFF) - 1) & 0x07;
                hdr[0] |= (((mac_size - 2) / 2) & 0x07) << 3;
                hdr[0] |= (aad_size > 0) ? 0x40 : 0x00;

                /* Encode the data size. */
                for (size_t i = 0; i < sizeof(s64); ++i) {
                    hdr[BlockSize - 1 - i] = static_cast<u64>(data_size) >> (BITSIZEOF(u8) * i);
                }

                /* Copy the nonce. */
                std::memcpy(hdr + 1, nonce, nonce_size);
            }

            void ProcessEncodedAadSize() {
                u8 encoded_aad[10];
                size_t encoded_aad_size;

                if (m_given_aad_size < ((1 << 16) - (1 << 8))) {
                    encoded_aad[0] = (m_given_aad_size >> (BITSIZEOF(u8) * 1)) & 0xFF;
                    encoded_aad[1] = (m_given_aad_size >> (BITSIZEOF(u8) * 0)) & 0xFF;
                    encoded_aad_size = 2;
                } else if (m_given_aad_size <= 0xFFFFFFFFu) {
                    encoded_aad[0] = 0xFF;
                    encoded_aad[1] = 0xFE;
                    encoded_aad[2] = (m_given_aad_size >> (BITSIZEOF(u8) * 3)) & 0xFF;
                    encoded_aad[3] = (m_given_aad_size >> (BITSIZEOF(u8) * 2)) & 0xFF;
                    encoded_aad[4] = (m_given_aad_size >> (BITSIZEOF(u8) * 1)) & 0xFF;
                    encoded_aad[5] = (m_given_aad_size >> (BITSIZEOF(u8) * 0)) & 0xFF;
                    encoded_aad_size = 6;
                } else {
                    encoded_aad[0] = 0xFF;
                    encoded_aad[1] = 0xFE;
                    encoded_aad[2] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 7)) & 0xFF;
                    encoded_aad[3] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 6)) & 0xFF;
                    encoded_aad[4] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 5)) & 0xFF;
                    encoded_aad[5] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 4)) & 0xFF;
                    encoded_aad[6] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 3)) & 0xFF;
                    encoded_aad[7] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 2)) & 0xFF;
                    encoded_aad[8] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 1)) & 0xFF;
                    encoded_aad[9] = (static_cast<u64>(m_given_aad_size) >> (BITSIZEOF(u8) * 0)) & 0xFF;
                    encoded_aad_size = 10;
                }

                m_cbc_mac_impl.template Update<BlockCipher>(encoded_aad, encoded_aad_size);
            }

            void ProcessPadding() {
                /* Process any remaining padding. */
                if (const auto buffered = m_cbc_mac_impl.GetBufferedDataSize(); buffered > 0) {
                    u8 zeros[BlockSize] = {};
                    m_cbc_mac_impl.template Update<BlockCipher>(zeros, BlockSize - buffered);
                }
            }

            void GenerateMac() {
                /* Get the cbc mac. */
                u8 tmp[BlockSize];
                m_cbc_mac_impl.GetMac(tmp, BlockSize);

                /* Xor into our mac. */
                for (size_t i = 0; i < BlockSize; ++i) {
                    m_mac[i] ^= tmp[i];
                }
            }
    };

}
