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

        struct RsaKeyInfo {
            int modulus_size_val;
            int exponent_size_val;
        };

        constinit RsaKeyInfo g_rsa_key_infos[RsaKeySlotCount] = {};

        void ClearRsaKeySlot(volatile SecurityEngineRegisters *SE, int slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL expmod) {
            constexpr int NumWords = se::RsaSize / sizeof(u32);
            for (int i = 0; i < NumWords; ++i) {
                /* Select the keyslot word. */
                reg::Write(SE->SE_RSA_KEYTABLE_ADDR, SE_REG_BITS_ENUM (RSA_KEYTABLE_ADDR_INPUT_MODE, REGISTER),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_KEY_SLOT,       slot),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_EXPMOD_SEL,   expmod),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_WORD_ADDR,         i));

                /* Clear the keyslot word. */
                SE->SE_RSA_KEYTABLE_DATA = 0;
            }
        }

        void SetRsaKey(volatile SecurityEngineRegisters *SE, int slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL expmod, const void *key, size_t key_size) {
            const int num_words = key_size / sizeof(u32);
            for (int i = 0; i < num_words; ++i) {
                /* Select the keyslot word. */
                reg::Write(SE->SE_RSA_KEYTABLE_ADDR, SE_REG_BITS_ENUM (RSA_KEYTABLE_ADDR_INPUT_MODE, REGISTER),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_KEY_SLOT,       slot),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_EXPMOD_SEL,   expmod),
                                                     SE_REG_BITS_VALUE(RSA_KEYTABLE_ADDR_WORD_ADDR,         i));

                /* Get the word. */
                const u32 word = util::LoadBigEndian(static_cast<const u32 *>(key) + (num_words - 1 - i));

                /* Write the keyslot word. */
                SE->SE_RSA_KEYTABLE_DATA = word;
            }
        }

        void GetRsaResult(volatile SecurityEngineRegisters *SE, void *dst, size_t size) {
            /* Copy out the words. */
            const int num_words = size / sizeof(u32);
            for (int i = 0; i < num_words; ++i) {
                const u32 word = reg::Read(SE->SE_RSA_OUTPUT[i]);
                util::StoreBigEndian(static_cast<u32 *>(dst) + num_words - 1 - i, word);
            }
        }

        void WaitForInputReadComplete(volatile SecurityEngineRegisters *SE) {
            while (reg::HasValue(SE->SE_INT_STATUS, SE_REG_BITS_ENUM(INT_STATUS_IN_DONE, CLEAR))) { /* ... */ }
        }

    }

    void ClearRsaKeySlot(int slot) {
        /* Validate the key slot. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);

        /* Clear the info. */
        g_rsa_key_infos[slot] = {};

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Clear the modulus. */
        ClearRsaKeySlot(SE, slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_MODULUS);

        /* Clear the exponent. */
        ClearRsaKeySlot(SE, slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_EXPONENT);
    }

    void LockRsaKeySlot(int slot, u32 flags) {
        /* Validate the key slot. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Set non per-key flags. */
        if ((flags & ~KeySlotLockFlags_PerKey) != 0) {
            /* Pack the flags into the expected format. */
            u32 value = 0;
            value |= ((flags & KeySlotLockFlags_KeyRead) == 0) ? (1u << 0) : 0;
            value |= ((flags & KeySlotLockFlags_KeyRead) == 0) ? (1u << 1) : 0;
            value |= ((flags & KeySlotLockFlags_KeyRead) == 0) ? (1u << 2) : 0;

            reg::Write(SE->SE_RSA_KEYTABLE_ACCESS[slot], SE_REG_BITS_ENUM_SEL(RSA_KEYTABLE_ACCESS_KEYREAD,   (flags & KeySlotLockFlags_KeyRead)  != 0, DISABLE, ENABLE),
                                                         SE_REG_BITS_ENUM_SEL(RSA_KEYTABLE_ACCESS_KEYUPDATE, (flags & KeySlotLockFlags_KeyWrite) != 0, DISABLE, ENABLE),
                                                         SE_REG_BITS_ENUM_SEL(RSA_KEYTABLE_ACCESS_KEYUSE,    (flags & KeySlotLockFlags_KeyUse)   != 0, DISABLE, ENABLE));
        }

        /* Set per-key flag. */
        if ((flags & KeySlotLockFlags_PerKey) != 0) {
            reg::ReadWrite(SE->SE_RSA_SECURITY_PERKEY, REG_BITS_VALUE(slot, 1, 0));
        }
    }

    void SetRsaKey(int slot, const void *mod, size_t mod_size, const void *exp, size_t exp_size) {
        /* Validate the key slot and sizes. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);
        AMS_ABORT_UNLESS(mod_size <= RsaSize);
        AMS_ABORT_UNLESS(exp_size <= RsaSize);

        /* Set the sizes in the info. */
        auto &info = g_rsa_key_infos[slot];
        info.modulus_size_val  = (mod_size / 64) - 1;
        info.exponent_size_val = (exp_size /  4);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Set the modulus and exponent. */
        SetRsaKey(SE, slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_MODULUS,  mod, mod_size);
        SetRsaKey(SE, slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_EXPONENT, exp, exp_size);
    }

    void ModularExponentiate(void *dst, size_t dst_size, int slot, const void *src, size_t src_size) {
        /* Validate the slot and sizes. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);
        AMS_ABORT_UNLESS(src_size <= RsaSize);
        AMS_ABORT_UNLESS(dst_size <= RsaSize);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Create a work buffer. */
        u8 work[RsaSize];
        util::ClearMemory(work, sizeof(work));

        /* Copy the input into the work buffer (reversing endianness). */
        const u8 *src_u8 = static_cast<const u8 *>(src);
        for (size_t i = 0; i < src_size; ++i) {
            work[src_size - 1 - i] = src_u8[i];
        }

        /* Flush the work buffer to ensure the SE sees correct results. */
        hw::FlushDataCache(work, sizeof(work));
        hw::DataSynchronizationBarrierInnerShareable();

        /* Configure the engine to perform RSA encryption. */
        reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM(CONFIG_ENC_MODE, AESMODE_KEY128),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_MODE, AESMODE_KEY128),
                                  SE_REG_BITS_ENUM(CONFIG_ENC_ALG,             RSA),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_ALG,             NOP),
                                  SE_REG_BITS_ENUM(CONFIG_DST,             RSA_REG));

        /* Configure the engine to use the keyslot and correct modulus/exp sizes. */
        const auto &info = g_rsa_key_infos[slot];
        reg::Write(SE->SE_RSA_CONFIG, SE_REG_BITS_VALUE(RSA_CONFIG_KEY_SLOT, slot));
        reg::Write(SE->SE_RSA_KEY_SIZE, info.modulus_size_val);
        reg::Write(SE->SE_RSA_EXP_SIZE, info.exponent_size_val);

        /* Execute the operation. */
        ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, work, src_size);

        /* Copy out the result. */
        GetRsaResult(SE, dst, dst_size);
    }

    void ModularExponentiateAsync(int slot, const void *src, size_t src_size, DoneHandler handler) {
        /* Validate the slot and size. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);
        AMS_ABORT_UNLESS(src_size <= RsaSize);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Create a work buffer. */
        u8 work[RsaSize];
        util::ClearMemory(work, sizeof(work));

        /* Copy the input into the work buffer (reversing endianness). */
        const u8 *src_u8 = static_cast<const u8 *>(src);
        for (size_t i = 0; i < src_size; ++i) {
            work[src_size - 1 - i] = src_u8[i];
        }

        /* Flush the work buffer to ensure the SE sees correct results. */
        hw::FlushDataCache(work, sizeof(work));
        hw::DataSynchronizationBarrierInnerShareable();

        /* Configure the engine to perform RSA encryption. */
        reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM(CONFIG_ENC_MODE, AESMODE_KEY128),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_MODE, AESMODE_KEY128),
                                  SE_REG_BITS_ENUM(CONFIG_ENC_ALG,             RSA),
                                  SE_REG_BITS_ENUM(CONFIG_DEC_ALG,             NOP),
                                  SE_REG_BITS_ENUM(CONFIG_DST,             RSA_REG));

        /* Configure the engine to use the keyslot and correct modulus/exp sizes. */
        const auto &info = g_rsa_key_infos[slot];
        reg::Write(SE->SE_RSA_CONFIG, SE_REG_BITS_VALUE(RSA_CONFIG_KEY_SLOT, slot));
        reg::Write(SE->SE_RSA_KEY_SIZE, info.modulus_size_val);
        reg::Write(SE->SE_RSA_EXP_SIZE, info.exponent_size_val);

        /* Set the done handler. */
        SetDoneHandler(SE, handler);

        /* Trigger the input operation. */
        StartInputOperation(SE, work, src_size);

        /* Wait for input to be read by the se. */
        WaitForInputReadComplete(SE);
    }

    void GetRsaResult(void *dst, size_t dst_size) {
        GetRsaResult(GetRegisters(), dst, dst_size);
    }

}
