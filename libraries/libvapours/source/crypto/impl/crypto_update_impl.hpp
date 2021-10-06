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
#include <vapours.hpp>

namespace ams::crypto::impl {

    template<typename Cipher, typename Self>
    void UpdateImpl(Self *self, const void *src, size_t src_size) {
        const size_t BlockSize = self->GetBlockSize();

        const u8 *src_u8 = static_cast<const u8 *>(src);
        size_t remaining = src_size;

        if (const size_t buffered = self->GetBufferedDataSize(); buffered > 0) {
            const size_t partial = std::min(BlockSize - buffered, remaining);

            self->ProcessPartialData(src_u8, partial);
            src_u8    += partial;
            remaining -= partial;
        }

        if (remaining >= BlockSize) {
            const size_t num_blocks = remaining / BlockSize;

            self->template ProcessBlocks<Cipher>(src_u8, num_blocks);

            const size_t processed = num_blocks * BlockSize;
            src_u8    += processed;
            remaining -= processed;
        }

        if (remaining > 0) {
            self->ProcessRemainingData(src_u8, remaining);
        }
    }

    template<typename Cipher, typename Self>
    size_t UpdateImpl(Self *self, void *dst, size_t dst_size, const void *src, size_t src_size) {
        AMS_UNUSED(dst_size);

        const size_t BlockSize = self->GetBlockSize();

        const u8 *src_u8 = static_cast<const u8 *>(src);
              u8 *dst_u8 = static_cast<u8 *>(dst);
        size_t remaining = src_size;
        size_t total_processed = 0;

        if (const size_t buffered = self->GetBufferedDataSize(); buffered > 0) {
            const size_t partial = std::min(BlockSize - buffered, remaining);

            const size_t processed = self->ProcessPartialData(dst_u8, src_u8, partial);

            dst_u8          += processed;
            total_processed += processed;

            src_u8    += partial;
            remaining -= partial;
        }

        if (remaining >= BlockSize) {
            const size_t num_blocks = remaining / BlockSize;
            const size_t input_size = num_blocks * BlockSize;

            const size_t processed = self->template ProcessBlocks<Cipher>(dst_u8, src_u8, num_blocks);

            dst_u8          += processed;
            total_processed += processed;

            src_u8    += input_size;
            remaining -= input_size;
        }

        if (remaining > 0) {
            const size_t processed = self->ProcessRemainingData(dst_u8, src_u8, remaining);
            total_processed += processed;
        }

        return total_processed;
    }



}
