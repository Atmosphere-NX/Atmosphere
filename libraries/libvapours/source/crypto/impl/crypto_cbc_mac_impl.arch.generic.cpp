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
#include "crypto_update_impl.hpp"

namespace ams::crypto::impl {

    void CbcMacImpl::UpdateGeneric(const void *data, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_state == State_Initialized);

        /* Update. */
        UpdateImpl<void>(this, data, size);
    }

    void CbcMacImpl::ProcessBlocksGeneric(const void *data, size_t num_blocks) {
        /* If we have a block remaining, process it. */
        if (m_buffered_bytes == BlockSize) {
            this->ProcessBlock(m_buffer);
            m_buffered_bytes = 0;
        }

        /* Process blocks. */
        const u8 *data8 = static_cast<const u8 *>(data);

        u8 block[BlockSize];
        while ((--num_blocks) > 0) {
            for (size_t i = 0; i < BlockSize; ++i) {
                block[i] = data8[i] ^ m_mac[i];
            }

            m_cipher_function(m_mac, block, m_cipher_context);

            data8 += BlockSize;
        }

        /* Process the last block. */
        std::memcpy(m_buffer, data8, BlockSize);
        m_buffered_bytes = BlockSize;
    }

    template<>
    void CbcMacImpl::Update<AesEncryptor128>(const void *data, size_t size) {
        this->UpdateGeneric(data, size);
    }


}
