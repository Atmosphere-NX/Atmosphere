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
#include "se_execute.hpp"

namespace ams::se {

    namespace {

        constinit const u8 FixedPattern[AesBlockSize] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
        };

        bool TestRegister(volatile u32 &r, u16 v) {
            return (static_cast<u16>(reg::Read(r))) == v;
        }

        void ExecuteContextSaveOperation(volatile SecurityEngineRegisters *SE, void *dst, size_t dst_size, const void *src, size_t src_size) {
            /* Save the output to a temporary buffer. */
            util::AlignedBuffer<hw::DataCacheLineSize, AesBlockSize> temp;
            AMS_ABORT_UNLESS(dst_size <= AesBlockSize);

            /* Ensure that the cpu and SE see consistent data. */
            if (src_size > 0) {
                hw::FlushDataCache(src, src_size);
                hw::DataSynchronizationBarrierInnerShareable();
            }
            if (dst_size > 0) {
                hw::FlushDataCache(temp, AesBlockSize);
                hw::DataSynchronizationBarrierInnerShareable();
            }

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_CTX_SAVE, temp, dst_size, src, src_size);

            /* Copy output from the operation, if any. */
            if (dst_size > 0) {
                hw::DataSynchronizationBarrierInnerShareable();
                hw::FlushDataCache(temp, AesBlockSize);
                hw::DataSynchronizationBarrierInnerShareable();

                std::memcpy(dst, temp, dst_size);
            }
        }

        void SaveContextBlock(volatile SecurityEngineRegisters *SE, void *dst) {
            /* Configure to encrypt a single block. */
            reg::Write(SE->SE_CRYPTO_LAST_BLOCK, 0);

            /* Execute the operation. */
            ExecuteContextSaveOperation(SE, dst, AesBlockSize, nullptr, 0);
        }

    }

    bool ValidateStickyBits(const StickyBits &bits) {
        /* Get the registers. */
        auto *SE = GetRegisters();

        /* Check SE_SECURITY. */
        if (!TestRegister(SE->SE_SE_SECURITY, bits.se_security)) { return false; }

        /* Check TZRAM_SECURITY. */
        if (!TestRegister(SE->SE_TZRAM_SECURITY, bits.tzram_security)) { return false; }

        /* Check CRYPTO_SECURITY_PERKEY. */
        if (!TestRegister(SE->SE_CRYPTO_SECURITY_PERKEY, bits.crypto_security_perkey)) { return false; }

        /* Check CRYPTO_KEYTABLE_ACCESS. */
        for (int i = 0; i < AesKeySlotCount; ++i) {
            if (!TestRegister(SE->SE_CRYPTO_KEYTABLE_ACCESS[i], bits.crypto_keytable_access[i])) { return false; }
        }

        /* Test RSA_SECURITY_PERKEY */
        if (!TestRegister(SE->SE_RSA_SECURITY_PERKEY, bits.rsa_security_perkey)) { return false; }

        /* Check RSA_KEYTABLE_ACCESS. */
        for (int i = 0; i < RsaKeySlotCount; ++i) {
            if (!TestRegister(SE->SE_RSA_KEYTABLE_ACCESS[i], bits.rsa_keytable_access[i])) { return false; }
        }

        /* All sticky bits are valid. */
        return true;
    }

    void SaveContext(Context *dst) {
        /* Get the registers. */
        auto *SE = GetRegisters();

        /* Generate a random srk. */
        GenerateSrk();

        /* Save a randomly-generated block. */
        {
            util::AlignedBuffer<hw::DataCacheLineSize, AesBlockSize> random_block;

            /* Flush the region we're about to fill to ensure consistency with the SE. */
            hw::FlushDataCache(random_block, AesBlockSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Generate random bytes. */
            GenerateRandomBytes(random_block, AesBlockSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Flush to ensure the CPU sees consistent data for the region. */
            hw::FlushDataCache(random_block, AesBlockSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Configure to encrypt the random block to memory. */
            reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM(CONFIG_ENC_MODE, AESMODE_KEY128),
                                      SE_REG_BITS_ENUM(CONFIG_DEC_MODE, AESMODE_KEY128),
                                      SE_REG_BITS_ENUM(CONFIG_ENC_ALG,         AES_ENC),
                                      SE_REG_BITS_ENUM(CONFIG_DEC_ALG,             NOP),
                                      SE_REG_BITS_ENUM(CONFIG_DST,              MEMORY));

            /* Configure to context save using memory as source. */
            reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM(CTX_SAVE_CONFIG_SRC, MEM));

            /* Configure to encrypt a single block. */
            reg::Write(SE->SE_CRYPTO_LAST_BLOCK, 0);

            /* Execute the operation. */
            ExecuteContextSaveOperation(SE, dst->random, AesBlockSize, random_block, AesBlockSize);
        }

        /* Save the sticky bits. */
        for (size_t i = 0; i < util::size(dst->sticky_bits); ++i) {
            /* Configure to encrypt the sticky bits block. */
            reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_SRC,              STICKY_BITS),
                                               SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_STICKY_WORD_QUAD,           i));

            /* Save the block. */
            SaveContextBlock(SE, dst->sticky_bits[i]);
        }

        /* Save the aes keytable. */
        {
            for (size_t key = 0; key < util::size(dst->aes_key); ++key) {
                for (auto part = 0; part < AesKeySlotPartCount; ++part) {
                    /* Configure to encrypt the part of the key. */
                    reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_SRC,           AES_KEYTABLE),
                                                       SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_AES_KEY_INDEX,          key),
                                                       SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_AES_WORD_QUAD,         part));

                    /* Save the block. */
                    SaveContextBlock(SE, dst->aes_key[key][part]);
                }
            }

            for (size_t key = 0; key < util::size(dst->aes_oiv); ++key) {
                /* Configure to encrypt the original iv. */
                reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_SRC,           AES_KEYTABLE),
                                                   SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_AES_KEY_INDEX,          key),
                                                   SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_AES_WORD_QUAD, ORIGINAL_IVS));

                /* Save the block. */
                SaveContextBlock(SE, dst->aes_oiv[key]);
            }

            for (size_t key = 0; key < util::size(dst->aes_uiv); ++key) {
                /* Configure to encrypt the updated iv. */
                reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_SRC,           AES_KEYTABLE),
                                                   SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_AES_KEY_INDEX,          key),
                                                   SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_AES_WORD_QUAD,  UPDATED_IVS));

                /* Save the block. */
                SaveContextBlock(SE, dst->aes_uiv[key]);
            }
        }

        /* Save the rsa keytable. */
        for (size_t key = 0; key < util::size(dst->rsa_key); ++key) {
            for (auto part = 0; part < RsaKeySlotPartCount; ++part) {
                /* Note that the parts are done in reverse order. */
                const auto part_index = RsaKeySlotPartCount - 1 - part;

                /* Determine a total key index. */
                const auto key_index = key * util::size(dst->rsa_key) + part_index;

                for (size_t block = 0; block < RsaSize / AesBlockSize; ++block) {
                    /* Configure to encrypt the part of the key. */
                    reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM (CTX_SAVE_CONFIG_SRC,           RSA_KEYTABLE),
                                                       SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_RSA_KEY_INDEX,    key_index),
                                                       SE_REG_BITS_VALUE(CTX_SAVE_CONFIG_RSA_WORD_QUAD,        block));

                    /* Save the block. */
                    SaveContextBlock(SE, dst->rsa_key[key][part][block]);
                }
            }
        }

        /* Save the fixed pattern. */
        {
            /* Configure to context save using memory as source. */
            reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM(CTX_SAVE_CONFIG_SRC, MEM));

            /* Configure to encrypt a single block. */
            reg::Write(SE->SE_CRYPTO_LAST_BLOCK, 0);

            /* Execute the operation. */
            ExecuteContextSaveOperation(SE, dst->fixed_pattern, AesBlockSize, FixedPattern, AesBlockSize);
        }

        /* Save the srk. */
        {
            /* Configure to context save using srk as source. */
            reg::Write(SE->SE_CTX_SAVE_CONFIG, SE_REG_BITS_ENUM(CTX_SAVE_CONFIG_SRC, SRK));

            /* Configure to encrypt a single block. */
            reg::Write(SE->SE_CRYPTO_LAST_BLOCK, 0);

            /* Execute the operation. */
            ExecuteContextSaveOperation(SE, nullptr, 0, nullptr, 0);
        }

        /* Perform a no-op context save operation. */
        {
            /* Configure to perform no-op. */
            reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM(CONFIG_ENC_ALG,             NOP),
                                      SE_REG_BITS_ENUM(CONFIG_DEC_ALG,             NOP));

            /* Execute the operation. */
            ExecuteContextSaveOperation(SE, nullptr, 0, nullptr, 0);
        }
    }

    void ValidateErrStatus() {
        /* Get the registers. */
        auto *SE = GetRegisters();

        /* Ensure there is no error status. */
        AMS_ABORT_UNLESS(reg::Read(SE->SE_ERR_STATUS) == 0);

        /* Ensure no error occurred. */
        AMS_ABORT_UNLESS(reg::HasValue(SE->SE_INT_STATUS, SE_REG_BITS_ENUM(INT_STATUS_ERR_STAT, CLEAR)));
    }

}
