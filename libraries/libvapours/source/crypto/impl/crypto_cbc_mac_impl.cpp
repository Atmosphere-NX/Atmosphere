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
#include <vapours.hpp>

namespace ams::crypto::impl {

    void CbcMacImpl::ProcessBlock(const void *data) {
        /* Procses the block. */
        const u8 *data8 = static_cast<const u8 *>(data);

        u8 block[BlockSize];
        for (size_t i = 0; i < BlockSize; ++i) {
            block[i] = data8[i] ^ m_mac[i];
        }

        m_cipher_function(m_mac, block, m_cipher_context);
    }

    void CbcMacImpl::ProcessPartialData(const void *data, size_t size) {
        /* Copy in the data. */
        std::memcpy(m_buffer + m_buffered_bytes, data, size);
        m_buffered_bytes += size;
    }

    void CbcMacImpl::ProcessRemainingData(const void *data, size_t size) {
        /* If we have a block remaining, process it. */
        if (m_buffered_bytes == BlockSize) {
            this->ProcessBlock(m_buffer);
            m_buffered_bytes = 0;
        }

        /* Copy the remaining data. */
        std::memcpy(m_buffer, data, size);
        m_buffered_bytes = size;
    }

    void CbcMacImpl::GetMac(void *mac, size_t mac_size) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Done);
        AMS_ASSERT(mac_size >= BlockSize);
        AMS_UNUSED(mac_size);

        /* Ensure we're done. */
        if (m_state == State_Initialized) {
            if (m_buffered_bytes == BlockSize) {
                this->ProcessBlock(m_buffer);
                m_buffered_bytes = 0;
            }
            m_state = State_Done;
        }

        /* Copy out the mac. */
        std::memcpy(mac, m_mac, sizeof(m_mac));
    }

    void CbcMacImpl::MaskBufferedData(const void *data, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_buffered_bytes == BlockSize);
        AMS_ASSERT(size == BlockSize);
        AMS_UNUSED(size);

        /* Mask the data. */
        for (size_t i = 0; i < BlockSize; ++i) {
            m_buffer[i] ^= static_cast<const u8 *>(data)[i];
        }
    }

}
