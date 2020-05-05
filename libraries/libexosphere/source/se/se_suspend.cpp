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

        bool TestRegister(volatile u32 &r, u16 v) {
            return (static_cast<u16>(reg::Read(r))) == v;
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

        /* Test RSA_SCEURITY_PERKEY */
        if (!TestRegister(SE->SE_CRYPTO_SECURITY_PERKEY, bits.rsa_security_perkey)) { return false; }

        /* Check RSA_KEYTABLE_ACCESS. */
        for (int i = 0; i < RsaKeySlotCount; ++i) {
            if (!TestRegister(SE->SE_RSA_KEYTABLE_ACCESS[i], bits.rsa_keytable_access[i])) { return false; }
        }

        /* All sticky bits are valid. */
        return true;
    }

}
