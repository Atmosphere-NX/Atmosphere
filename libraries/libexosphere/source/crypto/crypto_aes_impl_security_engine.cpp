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
#include <exosphere.hpp>

namespace ams::crypto::impl {

    namespace {

        constexpr bool IsSupportedKeySize(size_t size) {
            return size == 16 || size == 24 || size == 32;
        }

    }

    template<size_t KeySize>
    AesImpl<KeySize>::~AesImpl() {
        ClearMemory(this, sizeof(*this));
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::Initialize(const void *key, size_t key_size, bool is_encrypt) {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(key_size == sizeof(int));
        AMS_UNUSED(is_encrypt);

        /* Set the security engine keyslot. */
        this->slot = *static_cast<const int *>(key);
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(src_size >= BlockSize);
        AMS_ASSERT(dst_size >= BlockSize);

        if constexpr (KeySize == 16) {
            /* Aes 128. */
            se::EncryptAes128(dst, dst_size, this->slot, src, src_size);
        } else if constexpr (KeySize == 24) {
            /* Aes 192. */
            /* TODO: se::EncryptAes192(dst, dst_size, this->slot, src, src_size); */
            AMS_UNUSED(dst, dst_size, src, src_size);
        } else if constexpr (KeySize == 32) {
            /* Aes 256. */
            /* TODO: se::EncryptAes256(dst, dst_size, this->slot, src, src_size); */
            AMS_UNUSED(dst, dst_size, src, src_size);
        } else {
            /* Invalid key size. */
            static_assert(!std::is_same<AesImpl<KeySize>, AesImpl<KeySize>>::value);
        }
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(src_size >= BlockSize);
        AMS_ASSERT(dst_size >= BlockSize);

        if constexpr (KeySize == 16) {
            /* Aes 128. */
            se::DecryptAes128(dst, dst_size, this->slot, src, src_size);
        } else if constexpr (KeySize == 24) {
            /* Aes 192. */
            /* TODO: se::DecryptAes192(dst, dst_size, this->slot, src, src_size); */
            AMS_UNUSED(dst, dst_size, src, src_size);
        } else if constexpr (KeySize == 32) {
            /* Aes 256. */
            /* TODO: se::DecryptAes256(dst, dst_size, this->slot, src, src_size); */
            AMS_UNUSED(dst, dst_size, src, src_size);
        } else {
            /* Invalid key size. */
            static_assert(!std::is_same<AesImpl<KeySize>, AesImpl<KeySize>>::value);
        }
    }


    /* Explicitly instantiate the three supported key sizes. */
    template class AesImpl<16>;
    template class AesImpl<24>;
    template class AesImpl<32>;

}
