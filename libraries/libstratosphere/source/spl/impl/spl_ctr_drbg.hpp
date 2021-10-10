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
#include <stratosphere.hpp>

namespace ams::spl::impl {

    constexpr inline int BitsPerByte = BITSIZEOF(u8);

    /* Nintendo implements CTR_DRBG for their csrng. We will do the same. */
    template<typename BlockCipher, size_t KeySize, bool UseDerivation>
    class CtrDrbg {
        public:
            static constexpr int KeyLen                    = KeySize * BitsPerByte;
            static constexpr int OutLen                    = BlockCipher::BlockSize * BitsPerByte;
            static constexpr int SeedLen                   = KeyLen + OutLen;
            static constexpr int MaxNumberOfBitsPerRequest = (1 << 19);
            static constexpr int ReseedInterval            = 0x7FFFFFF0;

            static constexpr size_t OutSize        = OutLen / BitsPerByte;
            static constexpr size_t SeedSize       = SeedLen / BitsPerByte;
            static constexpr size_t RequestSizeMax = MaxNumberOfBitsPerRequest / BitsPerByte;

            static_assert(SeedSize % OutSize == 0);
        private:
            class Bcc {
                private:
                    u8 *m_buffer;
                    const BlockCipher *m_cipher;
                    size_t m_offset;
                public:
                    Bcc(u8 *buffer, const BlockCipher *cipher) : m_buffer(buffer), m_cipher(cipher), m_offset(0) { /* ... */ }

                    void Process(const void *data, size_t size) {
                        const u8 *data_8 = static_cast<const u8 *>(data);
                        size_t remaining = size;

                        while (m_offset + remaining >= OutSize) {
                            const size_t xor_size = OutSize - m_offset;

                            Xor(m_buffer + m_offset, data_8, xor_size);
                            m_cipher->EncryptBlock(m_buffer, OutSize, m_buffer, OutSize);

                            data_8    += xor_size;
                            remaining -= xor_size;

                            m_offset = 0;
                        }

                        Xor(m_buffer + m_offset, data_8, remaining);
                        m_offset += remaining;
                    }

                    void Flush() {
                        if (m_offset != 0) {
                            m_cipher->EncryptBlock(m_buffer, OutSize, m_buffer, OutSize);
                            m_offset = 0;
                        }
                    }
            };
        private:
            BlockCipher m_block_cipher;
            u8 m_v[OutSize];
            u8 m_key[KeySize];
            u8 m_work1[SeedSize];
            u8 m_work2[SeedSize];
            int m_reseed_counter;
        private:
            static void Xor(void *dst, const void *src, size_t size) {
                const u8 *src_u8 = static_cast<const u8 *>(src);
                      u8 *dst_u8 = static_cast<u8 *>(dst);

                for (size_t i = 0; i < size; i++) {
                    dst_u8[i] ^= src_u8[i];
                }
            }

            static void Increment(void *v) {
                u8 *v_8 = static_cast<u8 *>(v);

                for (int i = OutSize - 1; i >= 0; --i) {
                    if ((++v_8[i]) != 0) {
                        break;
                    }
                }
            }
        private:
            void DeriveSeed(void *seed, const void *a, size_t a_size, const void *b, size_t b_size, const void *c, size_t c_size) {
                /* Determine sizes. */
                const u32 in_size  = a_size + b_size + c_size;
                const u32 out_size = SeedSize;

                /* Create header/footer. */
                u32 header[2];
                util::StoreBigEndian(header + 0, in_size);
                util::StoreBigEndian(header + 1, out_size);
                const u8 footer = 0x80;

                /* Create seed as 000102... */
                u8 *seed_8 = static_cast<u8 *>(seed);
                for (size_t i = 0; i < KeySize; ++i) {
                    seed_8[i] = i;
                }

                /* Initialize block cipher. */
                m_block_cipher.Initialize(seed_8, KeySize);

                /* Perform derivation. */
                for (u32 block = 0; block < SeedSize / OutSize; ++block) {
                    /* Create the block index value. */
                    u32 block_value;
                    util::StoreBigEndian(std::addressof(block_value), block);

                    /* Get the target block. */
                    u8 *target = seed_8 + block * OutSize;
                    std::memset(target, 0, OutSize);

                    /* Create block processor. */
                    Bcc bcc(target, std::addressof(m_block_cipher));

                    /* Process block value. */
                    bcc.Process(std::addressof(block_value), sizeof(block_value));
                    bcc.Flush();

                    /* Process header/data. */
                    bcc.Process(header, sizeof(header));
                    bcc.Process(a, a_size);
                    bcc.Process(b, b_size);
                    bcc.Process(c, c_size);
                    bcc.Process(footer, std::addressof(footer));
                    bcc.Flush();
                }

                /* Initialize block cipher. */
                m_block_cipher.Initialize(seed_8, KeySize);

                /* Encrypt seed. */
                m_block_cipher.EncryptBlock(seed_8, OutSize, seed_8 + KeySize, OutSize);
                for (size_t offset = 0; offset < SeedSize - OutSize; offset += OutSize) {
                    m_block_cipher.EncryptBlock(seed_8 + offset + OutSize, OutSize, seed_8 + offset, OutSize);
                }
            }

            void UpdateStates(void *key, void *v, const void *provided_data) {
                /* Initialize block cipher. */
                m_block_cipher.Initialize(key, KeySize);

                /* Update work. */
                for (size_t offset = 0; offset < SeedSize; offset += OutSize) {
                    Increment(v);
                    m_block_cipher.EncryptBlock(std::addressof(m_work2[offset]), OutSize, v, OutSize);
                }

                /* Xor work with provided data. */
                Xor(m_work2, provided_data, SeedSize);

                /* Copy to key/v. */
                std::memcpy(key, m_work2 + 0, KeySize);
                std::memcpy(v,   m_work2 + KeySize, OutSize);
            }
        public:
            constexpr CtrDrbg() = default;

            void Initialize(const void *entropy, size_t entropy_size, const void *nonce, size_t nonce_size, const void *personalization, size_t personalization_size) {
                /* Handle init. */
                if constexpr (UseDerivation) {
                    this->DeriveSeed(m_work1, entropy, entropy_size, nonce, nonce_size, personalization, personalization_size);
                } else {
                    AMS_ASSERT(entropy_size         == SeedSize);
                    AMS_ASSERT(nonce_size           == 0);
                    AMS_ASSERT(personalization_size <= SeedSize);
                    AMS_UNUSED(entropy_size, nonce, nonce_size);

                    std::memcpy(m_work1, entropy, SeedSize);
                    Xor(m_work1, personalization, personalization_size);
                }

                /* Clear key/v. */
                std::memset(m_key, 0, sizeof(m_key));
                std::memset(m_v, 0, sizeof(m_v));

                /* Set key/v. */
                this->UpdateStates(m_key, m_v, m_work1);

                /* Set reseed counter. */
                m_reseed_counter = 1;
            }

            void Reseed(const void *entropy, size_t entropy_size, const void *addl, size_t addl_size) {
                /* Handle init. */
                if constexpr (UseDerivation) {
                    this->DeriveSeed(m_work1, entropy, entropy_size, addl, addl_size, nullptr, 0);
                } else {
                    AMS_ASSERT(entropy_size == SeedSize);
                    AMS_ASSERT(addl_size    <= SeedSize);
                    AMS_UNUSED(entropy_size);

                    std::memcpy(m_work1, entropy, SeedSize);
                    Xor(m_work1, addl, addl_size);
                }

                /* Set key/v. */
                this->UpdateStates(m_key, m_v, m_work1);

                /* Set reseed counter. */
                m_reseed_counter = 1;
            }

            bool Generate(void *out, size_t size, const void *addl, size_t addl_size) {
                /* Check that the request is small enough. */
                if (size > RequestSizeMax) {
                    return false;
                }

                /* Check if we need reseed. */
                if (m_reseed_counter > ReseedInterval) {
                    return false;
                }

                /* Clear work buffer. */
                std::memset(m_work1, 0, sizeof(m_work1));

                /* Process additional input, if we have any. */
                if (addl_size > 0) {
                    if constexpr (UseDerivation) {
                        this->DeriveSeed(m_work1, addl, addl_size, nullptr, 0, nullptr, 0);
                    } else {
                        AMS_ASSERT(addl_size <= SeedSize);
                        std::memcpy(m_work1, addl, addl_size);
                    }

                    /* Set key/v. */
                    this->UpdateStates(m_key, m_v, m_work1);
                }

                /* Get buffer and aligned size. */
                u8 *out_8 = static_cast<u8 *>(out);
                const size_t aligned_size = util::AlignDown(size, OutSize);

                /* Generate ctr bytes. */
                m_block_cipher.Initialize(m_key, KeySize);
                for (size_t offset = 0; offset < aligned_size; offset += OutSize) {
                    Increment(m_v);
                    m_block_cipher.EncryptBlock(out_8 + offset, OutSize, m_v, OutSize);
                }

                /* Handle any unaligned data. */
                if (size > aligned_size) {
                    u8 temp[OutSize];
                    Increment(m_v);
                    m_block_cipher.EncryptBlock(temp, sizeof(temp), m_v, OutSize);
                    std::memcpy(out_8 + aligned_size, temp, size - aligned_size);
                }

                /* Set key/v. */
                this->UpdateStates(m_key, m_v, m_work1);

                /* Increment reseed counter. */
                ++m_reseed_counter;
                return true;
            }
    };

}
