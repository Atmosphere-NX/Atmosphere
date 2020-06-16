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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/impl/crypto_hash_function.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>

namespace ams::crypto::impl {

    template<typename Hash> /* requires HashFunction<Hash> */
    class HmacImpl {
        NON_COPYABLE(HmacImpl);
        NON_MOVEABLE(HmacImpl);
        public:
            static constexpr size_t MacSize   = Hash::HashSize;
            static constexpr size_t BlockSize = Hash::BlockSize;
        private:
            static constexpr u32 IpadMagic = 0x36363636;
            static constexpr u32 OpadMagic = 0x5c5c5c5c;

            static constexpr u32 IpadMagicXorOpadMagic = IpadMagic ^ OpadMagic;
            static_assert(IpadMagicXorOpadMagic == 0x6a6a6a6a);
        private:
            enum State {
                State_None        = 0,
                State_Initialized = 1,
                State_Done        = 2,
            };
        private:
            Hash hash_function;
            u32 key[BlockSize / sizeof(u32)];
            u32 mac[MacSize   / sizeof(u32)];
            State state;
        public:
            HmacImpl() : state(State_None) { /* ... */ }
            ~HmacImpl() {
                static_assert(offsetof(HmacImpl, hash_function) == 0);

                /* Clear everything except for the hash function. */
                ClearMemory(reinterpret_cast<u8 *>(this) + sizeof(this->hash_function), sizeof(*this) - sizeof(this->hash_function));
            }

            void Initialize(const void *key, size_t key_size);
            void Update(const void *data, size_t data_size);
            void GetMac(void *dst, size_t dst_size);
    };

    template<typename Hash>
    inline void HmacImpl<Hash>::Initialize(const void *key, size_t key_size) {
        /* Clear the key storage. */
        std::memset(this->key, 0, sizeof(this->key));

        /* Set the key storage. */
        if (key_size > BlockSize) {
            this->hash_function.Initialize();
            this->hash_function.Update(key, key_size);
            this->hash_function.GetHash(this->key, this->hash_function.HashSize);
        } else {
            std::memcpy(this->key, key, key_size);
        }

        /* Xor the key with the ipad. */
        for (size_t i = 0; i < util::size(this->key); i++) {
            this->key[i] ^= IpadMagic;
        }

        /* Update the hash function with the xor'd key. */
        this->hash_function.Initialize();
        this->hash_function.Update(this->key, BlockSize);

        /* Mark initialized. */
        this->state = State_Initialized;
    }

    template<typename Hash>
    inline void HmacImpl<Hash>::Update(const void *data, size_t data_size) {
        AMS_ASSERT(this->state == State_Initialized);

        this->hash_function.Update(data, data_size);
    }

    template<typename Hash>
    inline void HmacImpl<Hash>::GetMac(void *dst, size_t dst_size) {
        AMS_ASSERT(this->state == State_Initialized || this->state == State_Done);
        AMS_ASSERT(dst_size >= MacSize);

        /* If we're not already finalized, get the final mac. */
        if (this->state == State_Initialized) {
            /* Get the hash of ((key ^ ipad) || data). */
            this->hash_function.GetHash(this->mac, MacSize);

            /* Xor the key with the opad. */
            for (size_t i = 0; i < util::size(this->key); i++) {
                this->key[i] ^= IpadMagicXorOpadMagic;
            }

            /* Calculate the final mac as hash of ((key ^ opad) || hash((key ^ ipad) || data)) */
            this->hash_function.Initialize();
            this->hash_function.Update(this->key, BlockSize);
            this->hash_function.Update(this->mac, MacSize);
            this->hash_function.GetHash(this->mac, MacSize);

            /* Set our state as done. */
            this->state = State_Done;
        }

        std::memcpy(dst, this->mac, MacSize);
    }

}
