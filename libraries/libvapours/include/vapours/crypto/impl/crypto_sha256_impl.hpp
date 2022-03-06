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

namespace ams::crypto {

    struct Sha256Context;

}

namespace ams::crypto::impl {

    class Sha256Impl {
        public:
            static constexpr size_t HashSize  = 0x20;
            static constexpr size_t BlockSize = 0x40;
        private:
            enum State {
                State_None,
                State_Initialized,
                State_Done,
            };
        private:
            u32 m_intermediate_hash[HashSize / sizeof(u32)];
            u8 m_buffer[BlockSize];
            size_t m_buffered_bytes;
            u64 m_bits_consumed;
            State m_state;
        public:
            Sha256Impl() : m_state(State_None) { /* ... */ }
            ~Sha256Impl() {
                ClearMemory(this, sizeof(*this));
            }

            void Initialize();
            void Update(const void *data, size_t size);
            void GetHash(void *dst, size_t size);

            void InitializeWithContext(const Sha256Context *context);
            size_t GetContext(Sha256Context *context) const;

            size_t GetBufferedDataSize() const { return m_buffered_bytes; }

            void GetBufferedData(void *dst, size_t dst_size) const {
                AMS_ASSERT(dst_size >= this->GetBufferedDataSize());
                AMS_UNUSED(dst_size);

                std::memcpy(dst, m_buffer, m_buffered_bytes);
            }
        private:
            void ProcessBlock(const void *data);
            void ProcessBlocks(const u8 *data, size_t block_count);
            void ProcessLastBlock();
    };

    static_assert(HashFunction<Sha256Impl>);

}
