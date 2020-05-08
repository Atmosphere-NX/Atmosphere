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
#include <stratosphere.hpp>
#include "spl_ctr_drbg.hpp"

namespace ams::spl {

    void CtrDrbg::Update(const void *data) {
        aes128ContextCreate(&this->aes_ctx, this->key, true);
        for (size_t offset = 0; offset < sizeof(this->work[1]); offset += BlockSize) {
            IncrementCounter(this->counter);
            aes128EncryptBlock(&this->aes_ctx, &this->work[1][offset], this->counter);
        }

        Xor(this->work[1], data, sizeof(this->work[1]));

        std::memcpy(this->key, &this->work[1][0], sizeof(this->key));
        std::memcpy(this->counter, &this->work[1][BlockSize], sizeof(this->key));
    }

    void CtrDrbg::Initialize(const void *seed) {
        std::memcpy(this->work[0], seed, sizeof(this->work[0]));
        std::memset(this->key, 0, sizeof(this->key));
        std::memset(this->counter, 0, sizeof(this->counter));
        this->Update(this->work[0]);
        this->reseed_counter = 1;
    }

    void CtrDrbg::Reseed(const void *seed) {
        std::memcpy(this->work[0], seed, sizeof(this->work[0]));
        this->Update(this->work[0]);
        this->reseed_counter = 1;
    }

    bool CtrDrbg::GenerateRandomBytes(void *out, size_t size) {
        if (size > MaxRequestSize) {
            return false;
        }

        if (this->reseed_counter > ReseedInterval) {
            return false;
        }

        aes128ContextCreate(&this->aes_ctx, this->key, true);
        u8 *cur_dst = reinterpret_cast<u8 *>(out);

        size_t aligned_size = (size & ~(BlockSize - 1));
        for (size_t offset = 0; offset < aligned_size; offset += BlockSize) {
            IncrementCounter(this->counter);
            aes128EncryptBlock(&this->aes_ctx, cur_dst, this->counter);
            cur_dst += BlockSize;
        }

        if (size > aligned_size) {
            IncrementCounter(this->counter);
            aes128EncryptBlock(&this->aes_ctx, this->work[1], this->counter);
            std::memcpy(cur_dst, this->work[1], size - aligned_size);
        }

        std::memset(this->work[0], 0, sizeof(this->work[0]));
        this->Update(this->work[0]);

        this->reseed_counter++;
        return true;

    }

}
