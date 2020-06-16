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

#ifdef ATMOSPHERE_IS_STRATOSPHERE

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
        AMS_ASSERT(key_size == KeySize);

        if constexpr (KeySize == 16) {
            /* Aes 128. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes128Context));
            aes128ContextCreate(reinterpret_cast<Aes128Context *>(this->round_keys), key, is_encrypt);
        } else if constexpr (KeySize == 24) {
            /* Aes 192. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes192Context));
            aes192ContextCreate(reinterpret_cast<Aes192Context *>(this->round_keys), key, is_encrypt);
        } else if constexpr (KeySize == 32) {
            /* Aes 256. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes256Context));
            aes256ContextCreate(reinterpret_cast<Aes256Context *>(this->round_keys), key, is_encrypt);
        } else {
            /* Invalid key size. */
            static_assert(!std::is_same<AesImpl<KeySize>, AesImpl<KeySize>>::value);
        }
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(src_size >= BlockSize);
        AMS_ASSERT(dst_size >= BlockSize);

        if constexpr (KeySize == 16) {
            /* Aes 128. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes128Context));
            aes128EncryptBlock(reinterpret_cast<const Aes128Context *>(this->round_keys), dst, src);
        } else if constexpr (KeySize == 24) {
            /* Aes 192. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes192Context));
            aes192EncryptBlock(reinterpret_cast<const Aes192Context *>(this->round_keys), dst, src);
        } else if constexpr (KeySize == 32) {
            /* Aes 256. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes256Context));
            aes256EncryptBlock(reinterpret_cast<const Aes256Context *>(this->round_keys), dst, src);
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
            static_assert(sizeof(this->round_keys) == sizeof(::Aes128Context));
            aes128DecryptBlock(reinterpret_cast<const Aes128Context *>(this->round_keys), dst, src);
        } else if constexpr (KeySize == 24) {
            /* Aes 192. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes192Context));
            aes192DecryptBlock(reinterpret_cast<const Aes192Context *>(this->round_keys), dst, src);
        } else if constexpr (KeySize == 32) {
            /* Aes 256. */
            static_assert(sizeof(this->round_keys) == sizeof(::Aes256Context));
            aes256DecryptBlock(reinterpret_cast<const Aes256Context *>(this->round_keys), dst, src);
        } else {
            /* Invalid key size. */
            static_assert(!std::is_same<AesImpl<KeySize>, AesImpl<KeySize>>::value);
        }
    }


    /* Explicitly instantiate the three supported key sizes. */
    template class AesImpl<16>;
    template class AesImpl<24>;
    template class AesImpl<32>;

#else

    /* NOTE: Exosphere defines this in libexosphere. */

    /* TODO: Non-EL0 implementation. */

#endif

}