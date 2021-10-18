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
            Hash m_hash_function;
            u32 m_key[BlockSize / sizeof(u32)];
            u32 m_mac[MacSize   / sizeof(u32)];
            State m_state;
        public:
            HmacImpl() : m_state(State_None) { /* ... */ }
            ~HmacImpl() {
                static_assert(AMS_OFFSETOF(HmacImpl, m_hash_function) == 0);

                /* Clear everything except for the hash function. */
                ClearMemory(reinterpret_cast<u8 *>(this) + sizeof(m_hash_function), sizeof(*this) - sizeof(m_hash_function));
            }

            void Initialize(const void *key, size_t key_size);
            void Update(const void *data, size_t data_size);
            void GetMac(void *dst, size_t dst_size);
    };

    template<typename Hash>
    inline void HmacImpl<Hash>::Initialize(const void *key, size_t key_size) {
        /* Clear the key storage. */
        std::memset(m_key, 0, sizeof(m_key));

        /* Set the key storage. */
        if (key_size > BlockSize) {
            m_hash_function.Initialize();
            m_hash_function.Update(key, key_size);
            m_hash_function.GetHash(m_key, m_hash_function.HashSize);
        } else {
            std::memcpy(m_key, key, key_size);
        }

        /* Xor the key with the ipad. */
        for (size_t i = 0; i < util::size(m_key); i++) {
            m_key[i] ^= IpadMagic;
        }

        /* Update the hash function with the xor'd key. */
        m_hash_function.Initialize();
        m_hash_function.Update(m_key, BlockSize);

        /* Mark initialized. */
        m_state = State_Initialized;
    }

    template<typename Hash>
    inline void HmacImpl<Hash>::Update(const void *data, size_t data_size) {
        AMS_ASSERT(m_state == State_Initialized);

        m_hash_function.Update(data, data_size);
    }

    template<typename Hash>
    inline void HmacImpl<Hash>::GetMac(void *dst, size_t dst_size) {
        AMS_ASSERT(m_state == State_Initialized || m_state == State_Done);
        AMS_ASSERT(dst_size >= MacSize);
        AMS_UNUSED(dst_size);

        /* If we're not already finalized, get the final mac. */
        if (m_state == State_Initialized) {
            /* Get the hash of ((key ^ ipad) || data). */
            m_hash_function.GetHash(m_mac, MacSize);

            /* Xor the key with the opad. */
            for (size_t i = 0; i < util::size(m_key); i++) {
                m_key[i] ^= IpadMagicXorOpadMagic;
            }

            /* Calculate the final mac as hash of ((key ^ opad) || hash((key ^ ipad) || data)) */
            m_hash_function.Initialize();
            m_hash_function.Update(m_key, BlockSize);
            m_hash_function.Update(m_mac, MacSize);
            m_hash_function.GetHash(m_mac, MacSize);

            /* Set our state as done. */
            m_state = State_Done;
        }

        std::memcpy(dst, m_mac, MacSize);
    }

}
