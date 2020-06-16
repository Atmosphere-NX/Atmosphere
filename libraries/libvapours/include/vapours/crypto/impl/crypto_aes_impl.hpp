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


namespace ams::crypto::impl {

    template<size_t _KeySize>
    class AesImpl {
        public:
            static constexpr size_t KeySize      = _KeySize;
            static constexpr size_t BlockSize    = 16;
            static constexpr s32    RoundCount   = (KeySize / 4) + 6;
            static constexpr size_t RoundKeySize = BlockSize * (RoundCount + 1);
        private:
        #ifdef ATMOSPHERE_IS_EXOSPHERE
            int slot;
        #endif
        #ifdef ATMOSPHERE_IS_STRATOSPHERE
            u32 round_keys[RoundKeySize / sizeof(u32)];
        #endif
        public:
            ~AesImpl();

            void Initialize(const void *key, size_t key_size, bool is_encrypt);
            void EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const;
            void DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const;

        #ifdef ATMOSPHERE_IS_STRATOSPHERE
            const u8 *GetRoundKey() const {
                return reinterpret_cast<const u8 *>(this->round_keys);
            }
        #endif
    };

    /* static_assert(HashFunction<Sha1Impl>); */

}
