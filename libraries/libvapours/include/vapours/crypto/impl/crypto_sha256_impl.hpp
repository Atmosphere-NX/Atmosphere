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

namespace ams::crypto {

    struct Sha256Context;

}

namespace ams::crypto::impl {

    class Sha256Impl {
        public:
            static constexpr size_t HashSize  = 0x20;
            static constexpr size_t BlockSize = 0x40;
        private:
            struct State {
                u32 intermediate_hash[HashSize / sizeof(u32)];
                u8  buffer[BlockSize];
                u64 bits_consumed;
                size_t num_buffered;
                bool finalized;
            };
        private:
            State state;
        public:
            Sha256Impl() { /* ... */ }
            ~Sha256Impl() {
                static_assert(std::is_trivially_destructible<State>::value);
                ClearMemory(std::addressof(this->state), sizeof(this->state));
            }

            void Initialize();
            void Update(const void *data, size_t size);
            void GetHash(void *dst, size_t size);

            void InitializeWithContext(const Sha256Context *context);
            size_t GetContext(Sha256Context *context) const;

            size_t GetBufferedDataSize() const { return this->state.num_buffered; }

            void GetBufferedData(void *dst, size_t dst_size) const {
                AMS_ASSERT(dst_size >= this->GetBufferedDataSize());

                std::memcpy(dst, this->state.buffer, this->GetBufferedDataSize());
            }
    };

    static_assert(HashFunction<Sha256Impl>);

}
