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

    class Md5Impl {
        public:
            static constexpr size_t HashSize       = 0x10;
            static constexpr size_t BlockSize      = 0x40;
        private:
            enum State {
                State_None        = 0,
                State_Initialized = 1,
                State_Done        = 2,
            };
        private:
            union {
                struct {
                    u32 a, b, c, d;
                } p;
                u32 state[4];
            } m_x;
            alignas(8) u8 m_y[BlockSize];
            size_t m_size;
            State m_state;
        public:
            Md5Impl() : m_state(State_None) { /* ... */ }
            ~Md5Impl() { ClearMemory(this, sizeof(*this)); }

            void Initialize();
            void Update(const void *data, size_t size);
            void GetHash(void *dst, size_t size);
        private:
            void ProcessBlock();
            void ProcessLastBlock();
    };

    /* static_assert(HashFunction<Md5Impl>); */

}
