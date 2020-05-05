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

        void ClearRsaKeySlot(int slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL expmod) {
            /* Get the engine. */
            auto *SE = GetRegisters();

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

        void SetRsaKey(int slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL expmod, const void *key, size_t key_size) {
            /* Get the engine. */
            auto *SE = GetRegisters();

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

    }

    void ClearRsaKeySlot(int slot) {
        /* Validate the key slot. */
        AMS_ABORT_UNLESS(0 <= slot && slot < RsaKeySlotCount);

        /* Clear the info. */
        g_rsa_key_infos[slot] = {};

        /* Clear the modulus. */
        ClearRsaKeySlot(slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_MODULUS);

        /* Clear the exponent. */
        ClearRsaKeySlot(slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_EXPONENT);
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

        /* Set the modulus and exponent. */
        SetRsaKey(slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_MODULUS,  mod, mod_size);
        SetRsaKey(slot, SE_RSA_KEYTABLE_ADDR_EXPMOD_SEL_EXPONENT, exp, exp_size);
    }

}
