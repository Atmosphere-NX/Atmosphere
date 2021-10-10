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
#include <stratosphere.hpp>
#include "spl_ctr_drbg.hpp"

namespace ams::spl {

    void CtrDrbg::Update(const void *data) {
        aes128ContextCreate(std::addressof(m_aes_ctx), m_key, true);
        for (size_t offset = 0; offset < sizeof(m_work[1]); offset += BlockSize) {
            IncrementCounter(m_counter);
            aes128EncryptBlock(std::addressof(m_aes_ctx), std::addressof(m_work[1][offset]), m_counter);
        }

        Xor(m_work[1], data, sizeof(m_work[1]));

        std::memcpy(m_key, std::addressof(m_work[1][0]), sizeof(m_key));
        std::memcpy(m_counter, std::addressof(m_work[1][BlockSize]), sizeof(m_key));
    }

    void CtrDrbg::Initialize(const void *seed) {
        std::memcpy(m_work[0], seed, sizeof(m_work[0]));
        std::memset(m_key, 0, sizeof(m_key));
        std::memset(m_counter, 0, sizeof(m_counter));
        this->Update(m_work[0]);
        m_reseed_counter = 1;
    }

    void CtrDrbg::Reseed(const void *seed) {
        std::memcpy(m_work[0], seed, sizeof(m_work[0]));
        this->Update(m_work[0]);
        m_reseed_counter = 1;
    }

    bool CtrDrbg::GenerateRandomBytes(void *out, size_t size) {
        if (size > MaxRequestSize) {
            return false;
        }

        if (m_reseed_counter > ReseedInterval) {
            return false;
        }

        aes128ContextCreate(std::addressof(m_aes_ctx), m_key, true);
        u8 *cur_dst = reinterpret_cast<u8 *>(out);

        size_t aligned_size = (size & ~(BlockSize - 1));
        for (size_t offset = 0; offset < aligned_size; offset += BlockSize) {
            IncrementCounter(m_counter);
            aes128EncryptBlock(std::addressof(m_aes_ctx), cur_dst, m_counter);
            cur_dst += BlockSize;
        }

        if (size > aligned_size) {
            IncrementCounter(m_counter);
            aes128EncryptBlock(std::addressof(m_aes_ctx), m_work[1], m_counter);
            std::memcpy(cur_dst, m_work[1], size - aligned_size);
        }

        std::memset(m_work[0], 0, sizeof(m_work[0]));
        this->Update(m_work[0]);

        m_reseed_counter++;
        return true;

    }

}
