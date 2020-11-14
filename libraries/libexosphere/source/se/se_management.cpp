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

        constinit uintptr_t g_register_address  = secmon::MemoryRegionPhysicalDeviceSecurityEngine.GetAddress();
        constinit uintptr_t g_register2_address = secmon::MemoryRegionPhysicalDeviceSecurityEngine2.GetAddress();
        constinit DoneHandler g_done_handler   = nullptr;

        void SetSecure(volatile SecurityEngineRegisters *SE, bool secure) {
            /* Set the security software setting. */
            if (secure) {
                reg::ReadWrite(SE->SE_SE_SECURITY, SE_REG_BITS_ENUM(SECURITY_SOFT_SETTING, SECURE));
            } else {
                reg::ReadWrite(SE->SE_SE_SECURITY,  SE_REG_BITS_ENUM(SECURITY_SOFT_SETTING, NONSECURE));
            }

            /* Read the status register to force an update. */
            reg::Read(SE->SE_SE_SECURITY);
        }

    }

    volatile SecurityEngineRegisters *GetRegisters() {
        return reinterpret_cast<volatile SecurityEngineRegisters *>(g_register_address);
    }

    volatile SecurityEngineRegisters *GetRegisters2() {
        return reinterpret_cast<volatile SecurityEngineRegisters *>(g_register2_address);
    }

    void SetRegisterAddress(uintptr_t address, uintptr_t address2) {
        g_register_address  = address;
        g_register2_address = address2;
    }

    void Initialize() {
        auto *SE = GetRegisters();
        AMS_ABORT_UNLESS(reg::HasValue(SE->SE_STATUS, SE_REG_BITS_ENUM(STATUS_STATE, IDLE)));
    }

    void SetSecure(bool secure) {
        /* Set security for SE1. */
        SetSecure(GetRegisters(), secure);

        /* If SE2 is present, set security for SE2. */
        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            SetSecure(GetRegisters2(), secure);
        }
    }

    void SetTzramSecure() {
        auto *SE = GetRegisters();

        /* Set the TZRAM setting to secure. */
        SE->SE_TZRAM_SECURITY = SE_TZRAM_SETTING_SECURE;
    }

    void SetPerKeySecure() {
        auto *SE = GetRegisters();

        /* Update PERKEY_SETTING to secure. */
        reg::ReadWrite(SE->SE_SE_SECURITY, SE_REG_BITS_ENUM(SECURITY_PERKEY_SETTING, SECURE));
    }


    void SetContextSaveSecure() {
        /* Context save lock to trustzone secure is only available on mariko. */
        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            auto *SE  = GetRegisters();
            auto *SE2 = GetRegisters2();

            reg::ReadWrite(SE->SE_SE_SECURITY,  SE_REG_BITS_ENUM(SECURITY_CTX_SAVE_TZ_LOCK, SECURE));
            reg::ReadWrite(SE2->SE_SE_SECURITY, SE_REG_BITS_ENUM(SECURITY_CTX_SAVE_TZ_LOCK, SECURE));
        }
    }

    void Lockout() {
        auto *SE = GetRegisters();

        /* Lock access to the AES keyslots. */
        for (int i = 0; i < AesKeySlotCount; ++i) {
            SE->SE_CRYPTO_KEYTABLE_ACCESS[i] = 0;
        }

        /* Lock access to the RSA keyslots. */
        for (int i = 0; i < RsaKeySlotCount; ++i) {
            SE->SE_RSA_KEYTABLE_ACCESS[i] = 0;
        }

        /* Set Per Key secure. */
        SetPerKeySecure();

        /* Configure SE_SECURITY. */
        {
            reg::ReadWrite(SE->SE_SE_SECURITY, SE_REG_BITS_ENUM(SECURITY_HARD_SETTING,   SECURE),
                                               SE_REG_BITS_ENUM(SECURITY_ENG_DIS,        DISABLE),
                                               SE_REG_BITS_ENUM(SECURITY_PERKEY_SETTING, SECURE),
                                               SE_REG_BITS_ENUM(SECURITY_SOFT_SETTING,   SECURE));
        }
    }

    void HandleInterrupt() {
        /* Get the registers. */
        auto *SE = GetRegisters();

        /* Disable the SE interrupt. */
        reg::Write(SE->SE_INT_ENABLE, 0);

        /* Execute the handler if we have one. */
        if (const auto handler = g_done_handler; handler != nullptr) {
            g_done_handler = nullptr;
            handler();
        }
    }

    void SetDoneHandler(volatile SecurityEngineRegisters *SE, DoneHandler handler) {
        /* Set the done handler. */
        g_done_handler = handler;

        /* Configure to trigger an interrupt when done. */
        reg::Write(SE->SE_INT_ENABLE, SE_REG_BITS_ENUM(INT_ENABLE_SE_OP_DONE, ENABLE));
    }

}
