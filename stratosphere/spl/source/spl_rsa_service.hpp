/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "spl_types.hpp"
#include "spl_crypto_service.hpp"

class RsaService : public CryptoService {
    public:
        RsaService(SecureMonitorWrapper *sw) : CryptoService(sw) {
            /* ... */
        }

        virtual ~RsaService() {
            /* ... */
        }
    protected:
        /* Actual commands. */
        virtual Result DecryptRsaPrivateKeyDeprecated(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
        virtual Result DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMetaEx<Spl_Cmd_GetConfig, &RsaService::GetConfig, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ExpMod, &RsaService::ExpMod, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetConfig, &RsaService::SetConfig, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateRandomBytes, &RsaService::GenerateRandomBytes, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_IsDevelopment, &RsaService::IsDevelopment, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetBootReason, &RsaService::SetBootReason, RsaService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetBootReason, &RsaService::GetBootReason, RsaService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKek, &RsaService::GenerateAesKek, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadAesKey, &RsaService::LoadAesKey, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKey, &RsaService::GenerateAesKey, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptAesKey, &RsaService::DecryptAesKey, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_CryptAesCtr, &RsaService::CryptAesCtr, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ComputeCmac, &RsaService::ComputeCmac, RsaService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_AllocateAesKeyslot, &RsaService::AllocateAesKeyslot, RsaService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_FreeAesKeyslot, &RsaService::FreeAesKeyslot, RsaService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetAesKeyslotAvailableEvent, &RsaService::GetAesKeyslotAvailableEvent, RsaService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &RsaService::DecryptRsaPrivateKeyDeprecated, RsaService, FirmwareVersion_400, FirmwareVersion_400>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &RsaService::DecryptRsaPrivateKey, RsaService, FirmwareVersion_500>(),

        };
};
