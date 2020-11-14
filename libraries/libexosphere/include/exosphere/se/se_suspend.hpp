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
#include <vapours.hpp>
#include <exosphere/se/se_aes.hpp>
#include <exosphere/se/se_rsa.hpp>

namespace ams::se {

    /* 256-bit AES keyslots are two 128-bit keys. */
    constexpr inline int AesKeySlotPartCount = 2;

    /* RSA keys are both a modulus and an exponent. */
    constexpr inline int RsaKeySlotPartCount = 2;

    constexpr inline size_t StickyBitContextSize = 2 * AesBlockSize;

    struct Context {
        u8 random[AesBlockSize];
        u8 sticky_bits[StickyBitContextSize / AesBlockSize][AesBlockSize];
        u8 aes_key[AesKeySlotCount][AesKeySlotPartCount][AesBlockSize];
        u8 aes_oiv[AesKeySlotCount][AesBlockSize];
        u8 aes_uiv[AesKeySlotCount][AesBlockSize];
        u8 rsa_key[RsaKeySlotCount][RsaKeySlotPartCount][RsaSize / AesBlockSize][AesBlockSize];
        u8 fixed_pattern[AesBlockSize];
    };
    static_assert(sizeof(Context) == 0x840);
    static_assert(util::is_pod<Context>::value);

    struct StickyBits {
        u8 se_security;
        u8 tzram_security;
        u16 crypto_security_perkey;
        u8 crypto_keytable_access[AesKeySlotCount];
        u8 rsa_security_perkey;
        u8 rsa_keytable_access[RsaKeySlotCount];
    };
    static_assert(util::is_pod<StickyBits>::value);

    bool ValidateStickyBits(const StickyBits &bits);
    void SaveContext(Context *dst);

    void ConfigureAutomaticContextSave();
    void SaveContextAutomatic();
    void SaveTzramAutomatic();

}
