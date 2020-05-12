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

        void SetMessageSize(volatile SecurityEngineRegisters *SE, size_t src_size) {
            /* Set the message size. */
            reg::Write(SE->SE_SHA_MSG_LENGTH[0], src_size * BITSIZEOF(u8));
            reg::Write(SE->SE_SHA_MSG_LENGTH[1], 0);
            reg::Write(SE->SE_SHA_MSG_LENGTH[2], 0);
            reg::Write(SE->SE_SHA_MSG_LENGTH[3], 0);

            /* Set the message remaining size. */
            reg::Write(SE->SE_SHA_MSG_LEFT[0], src_size * BITSIZEOF(u8));
            reg::Write(SE->SE_SHA_MSG_LEFT[1], 0);
            reg::Write(SE->SE_SHA_MSG_LEFT[2], 0);
            reg::Write(SE->SE_SHA_MSG_LEFT[3], 0);
        }

        void GetHashResult(volatile SecurityEngineRegisters *SE, void *dst, size_t dst_size) {
            /* Copy out the words. */
            const int num_words = dst_size / sizeof(u32);
            for (int i = 0; i < num_words; ++i) {
                const u32 word = reg::Read(SE->SE_HASH_RESULT[i]);
                util::StoreBigEndian(static_cast<u32 *>(dst) + i, word);
            }
        }

    }

    void CalculateSha256(Sha256Hash *dst, const void *src, size_t src_size) {
        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Configure the engine to perform SHA256 "encryption". */
        reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM(CONFIG_ENC_MODE,         SHA256),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_MODE, AESMODE_KEY128),
                                  SE_REG_BITS_ENUM(CONFIG_ENC_ALG,             SHA),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_ALG,             NOP),
                                  SE_REG_BITS_ENUM(CONFIG_DST,            HASH_REG));

        /* Begin a hardware hash operation. */
        reg::Write(SE->SE_SHA_CONFIG, SE_REG_BITS_VALUE(SHA_CONFIG_HW_INIT_HASH, 1));

        /* Set the message size. */
        SetMessageSize(SE, src_size);

        /* Execute the operation. */
        ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, src, src_size);

        /* Get the result. */
        GetHashResult(SE, dst, sizeof(*dst));
    }

}
