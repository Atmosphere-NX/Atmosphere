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
#include <vapours.hpp>

namespace ams::crypto::impl {

    namespace {

        /* TODO: Support non-Nintendo Endianness */

        void MultiplyTweakGeneric(u64 *tweak) {
            const u64 carry = tweak[1] & (static_cast<u64>(1) << (BITSIZEOF(u64) - 1));

            tweak[1] = ((tweak[1] << 1) | (tweak[0] >> (BITSIZEOF(u64) - 1)));
            tweak[0] = (tweak[0] << 1);

            if (carry) {
                tweak[0] ^= static_cast<u64>(0x87);
            }
        }

    }

    void XtsModeImpl::ProcessBlock(u8 *dst, const u8 *src) {
        u8 tmp[BlockSize];

        /* Xor. */
        for (size_t i = 0; i < BlockSize; i++) {
            tmp[i] = this->tweak[i] ^ src[i];
        }

        /* Crypt */
        this->cipher_func(tmp, tmp, this->cipher_ctx);

        /* Xor. */
        for (size_t i = 0; i < BlockSize; i++) {
            dst[i] = this->tweak[i] ^ tmp[i];
        }

        MultiplyTweakGeneric(reinterpret_cast<u64 *>(this->tweak));
    }

    size_t XtsModeImpl::FinalizeEncryption(void *dst, size_t dst_size) {
        AMS_ASSERT(this->state == State_Processing);
        AMS_UNUSED(dst_size);

        u8 *dst_u8 = static_cast<u8 *>(dst);
        size_t processed = 0;

        if (this->num_buffered == 0) {
            this->ProcessBlock(dst_u8, this->last_block);
            processed = BlockSize;
        } else {
            this->ProcessBlock(this->last_block, this->last_block);

            std::memcpy(this->buffer + this->num_buffered, this->last_block + this->num_buffered, BlockSize - this->num_buffered);

            this->ProcessBlock(dst_u8, this->buffer);

            std::memcpy(dst_u8 + BlockSize, this->last_block, this->num_buffered);

            processed = BlockSize + this->num_buffered;
        }

        this->state = State_Done;
        return processed;
    }

    size_t XtsModeImpl::FinalizeDecryption(void *dst, size_t dst_size) {
        AMS_ASSERT(this->state == State_Processing);
        AMS_UNUSED(dst_size);

        u8 *dst_u8 = static_cast<u8 *>(dst);
        size_t processed = 0;

        if (this->num_buffered == 0) {
            this->ProcessBlock(dst_u8, this->last_block);
            processed = BlockSize;
        } else {
            u8 tmp_tweak[BlockSize];
            std::memcpy(tmp_tweak, this->tweak, BlockSize);
            MultiplyTweakGeneric(reinterpret_cast<u64 *>(this->tweak));

            this->ProcessBlock(this->last_block, this->last_block);

            std::memcpy(this->buffer + this->num_buffered, this->last_block + this->num_buffered, BlockSize - this->num_buffered);

            std::memcpy(this->tweak, tmp_tweak, BlockSize);

            this->ProcessBlock(dst_u8, this->buffer);

            std::memcpy(dst_u8 + BlockSize, this->last_block, this->num_buffered);

            processed = BlockSize + this->num_buffered;
        }

        this->state = State_Done;
        return processed;
    }

    size_t XtsModeImpl::ProcessPartialData(u8 *dst, const u8 *src, size_t size) {
        size_t processed = 0;

        std::memcpy(this->buffer + this->num_buffered, src, size);
        this->num_buffered += size;

        if (this->num_buffered == BlockSize) {
            if (this->state == State_Processing) {
                this->ProcessBlock(dst, this->last_block);
                processed += BlockSize;
            }

            std::memcpy(this->last_block, this->buffer, BlockSize);
            this->num_buffered = 0;

            this->state = State_Processing;
        }

        return processed;
    }

    size_t XtsModeImpl::ProcessRemainingData(u8 *dst, const u8 *src, size_t size) {
        AMS_UNUSED(dst);

        std::memcpy(this->buffer, src, size);
        this->num_buffered = size;

        return 0;
    }

}
