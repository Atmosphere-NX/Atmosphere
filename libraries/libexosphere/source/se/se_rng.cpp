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

        constexpr inline int RngReseedInterval = 70001;

        void ConfigRng(volatile SecurityEngineRegisters *SE, SE_CONFIG_DST dst, SE_RNG_CONFIG_MODE mode) {
            /* Configure the engine to do RNG encryption. */
            reg::Write(SE->SE_CONFIG,  SE_REG_BITS_ENUM (CONFIG_ENC_MODE, AESMODE_KEY128),
                                       SE_REG_BITS_ENUM (CONFIG_DEC_MODE, AESMODE_KEY128),
                                       SE_REG_BITS_ENUM (CONFIG_ENC_ALG,             RNG),
                                       SE_REG_BITS_ENUM (CONFIG_DEC_ALG,             NOP),
                                       SE_REG_BITS_VALUE(CONFIG_DST,                 dst));

            reg::Write(SE->SE_CRYPTO_CONFIG, SE_REG_BITS_ENUM (CRYPTO_CONFIG_MEMIF,              AHB),
                                             SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,             0),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,  DISABLE),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_CORE_SEL,       ENCRYPT),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,     ORIGINAL),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,      MEMORY),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,       RANDOM),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,         BYPASS),
                                             SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,       DISABLE));

            /* Configure the RNG to use Entropy as source. */
            reg::Write(SE->SE_RNG_CONFIG, SE_REG_BITS_ENUM(RNG_CONFIG_SRC, ENTROPY), SE_REG_BITS_VALUE(RNG_CONFIG_MODE, mode));
        }

        void InitializeRandom(volatile SecurityEngineRegisters *SE) {
            /* Lock the entropy source. */
            reg::Write(SE->SE_RNG_SRC_CONFIG, SE_REG_BITS_ENUM(RNG_SRC_CONFIG_RO_ENTROPY_SOURCE,      ENABLE),
                                              SE_REG_BITS_ENUM(RNG_SRC_CONFIG_RO_ENTROPY_SOURCE_LOCK, ENABLE));

            /* Set the reseed interval to force a reseed every 70000 blocks. */
            SE->SE_RNG_RESEED_INTERVAL = RngReseedInterval;

            /* Initialize the DRBG. */
            {
                u8 dummy_buf[AesBlockSize];

                /* Configure the engine to force drbg instantiation by writing random to memory. */
                ConfigRng(SE, SE_CONFIG_DST_MEMORY, SE_RNG_CONFIG_MODE_FORCE_INSTANTIATION);

                /* Configure to do a single RNG block operation to trigger DRBG init. */
                SE->SE_CRYPTO_LAST_BLOCK = 0;

                /* Execute the operation. */
                ExecuteOperation(SE, SE_OPERATION_OP_START, dummy_buf, sizeof(dummy_buf), nullptr, 0);
            }
        }

        void GenerateSrk(volatile SecurityEngineRegisters *SE) {
            /* Configure the RNG to output to SRK and force a reseed. */
            ConfigRng(SE, SE_CONFIG_DST_SRK, SE_RNG_CONFIG_MODE_FORCE_RESEED);

            /* Configure a single block operation. */
            SE->SE_CRYPTO_LAST_BLOCK = 0;

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, nullptr, 0);
        }

    }

    void InitializeRandom() {
        /* Initialize random for SE1. */
        InitializeRandom(GetRegisters());

        /* If we have SE2, initialize random for SE2. */
        /* NOTE: Nintendo's implementation of this is incorrect. */
        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            InitializeRandom(GetRegisters2());
        }
    }

    void GenerateRandomBytes(void *dst, size_t size) {
        /* If we're not generating any bytes, there's nothing to do. */
        if (size == 0) {
            return;
        }

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Determine how many blocks to generate. */
        const size_t num_blocks   = size / AesBlockSize;
        const size_t aligned_size = num_blocks * AesBlockSize;
        const size_t fractional   = size - aligned_size;

        /* Configure the RNG to generate random to memory. */
        ConfigRng(SE, SE_CONFIG_DST_MEMORY, SE_RNG_CONFIG_MODE_NORMAL);

        /* Generate as many aligned blocks as we can. */
        if (aligned_size > 0) {
            /* Configure the engine to generate the right number of blocks. */
            SE->SE_CRYPTO_LAST_BLOCK = num_blocks - 1;

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, dst, aligned_size, nullptr, 0);
        }

        /* Generate a single block to output. */
        if (fractional > 0) {
            ExecuteOperationSingleBlock(SE, static_cast<u8 *>(dst) + aligned_size, fractional, nullptr, 0);
        }
    }

    void SetRandomKey(int slot) {
        /* NOTE: Nintendo does not validate the destination keyslot here. */

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Configure the RNG to output to the keytable. */
        ConfigRng(SE, SE_CONFIG_DST_KEYTABLE, SE_RNG_CONFIG_MODE_NORMAL);

        /* Configure the keytable destination to be the low part of the key. */
        reg::Write(SE->SE_CRYPTO_KEYTABLE_DST, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_DST_KEY_INDEX, slot), SE_REG_BITS_ENUM(CRYPTO_KEYTABLE_DST_WORD_QUAD, KEYS_0_3));

        /* Configure a single block operation. */
        SE->SE_CRYPTO_LAST_BLOCK = 0;

        /* Execute the operation to generate a random low-part of the key. */
        ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, nullptr, 0);

        /* Configure the keytable destination to be the high part of the key. */
        reg::Write(SE->SE_CRYPTO_KEYTABLE_DST, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_DST_KEY_INDEX, slot), SE_REG_BITS_ENUM(CRYPTO_KEYTABLE_DST_WORD_QUAD, KEYS_4_7));

        /* Execute the operation to generate a random high-part of the key. */
        ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, nullptr, 0);
    }

    void GenerateSrk() {
        /* Generate SRK for SE1. */
        GenerateSrk(GetRegisters());

        /* If we have SE2, generate SRK for SE2. */
        /* NOTE: Nintendo's implementation of this is incorrect. */
        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            GenerateSrk(GetRegisters2());
        }
    }

}
