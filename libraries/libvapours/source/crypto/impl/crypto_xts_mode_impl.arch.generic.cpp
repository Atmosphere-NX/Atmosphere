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

    size_t XtsModeImpl::UpdateGeneric(void *dst, size_t dst_size, const void *src, size_t src_size) {
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Processing);

        return UpdateImpl<void>(this, dst, dst_size, src, src_size);
    }

    size_t XtsModeImpl::ProcessBlocksGeneric(u8 *dst, const u8 *src, size_t num_blocks) {
        size_t processed = BlockSize * (num_blocks - 1);

        if (m_state == State_Processing) {
            this->ProcessBlock(dst, m_last_block);
            dst       += BlockSize;
            processed += BlockSize;
        }

        while ((--num_blocks) > 0) {
            this->ProcessBlock(dst, src);
            dst += BlockSize;
            src += BlockSize;
        }

        std::memcpy(m_last_block, src, BlockSize);

        m_state = State_Processing;

        return processed;
    }

    template<> size_t XtsModeImpl::Update<AesEncryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesEncryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesEncryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }

    template<> size_t XtsModeImpl::Update<AesDecryptor128>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesDecryptor192>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }
    template<> size_t XtsModeImpl::Update<AesDecryptor256>(void *dst, size_t dst_size, const void *src, size_t src_size) { return this->UpdateGeneric(dst, dst_size, src, src_size); }

}
