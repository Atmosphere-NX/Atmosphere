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
#include "spl_rsa_service.hpp"

class SslService : public RsaService {
    public:
        SslService(SecureMonitorWrapper *sw) : RsaService(sw) {
            /* ... */
        }

        virtual ~SslService() {
            /* ... */
        }
    protected:
        /* Actual commands. */
        virtual Result ImportSslKey(InPointer<u8> src, AccessKey access_key, KeySource key_source);
        virtual Result SslExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMetaEx<Spl_Cmd_GetConfig, &SslService::GetConfig, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ExpMod, &SslService::ExpMod, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetConfig, &SslService::SetConfig, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateRandomBytes, &SslService::GenerateRandomBytes, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_IsDevelopment, &SslService::IsDevelopment, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetBootReason, &SslService::SetBootReason, SslService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetBootReason, &SslService::GetBootReason, SslService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKek, &SslService::GenerateAesKek, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadAesKey, &SslService::LoadAesKey, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKey, &SslService::GenerateAesKey, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptAesKey, &SslService::DecryptAesKey, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_CryptAesCtr, &SslService::CryptAesCtr, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ComputeCmac, &SslService::ComputeCmac, SslService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_AllocateAesKeyslot, &SslService::AllocateAesKeyslot, SslService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_FreeAesKeyslot, &SslService::FreeAesKeyslot, SslService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetAesKeyslotAvailableEvent, &SslService::GetAesKeyslotAvailableEvent, SslService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &SslService::DecryptRsaPrivateKeyDeprecated, SslService, FirmwareVersion_400, FirmwareVersion_400>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &SslService::DecryptRsaPrivateKey, SslService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ImportSslKey, &SslService::ImportSslKey, SslService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SslExpMod, &SslService::SslExpMod, SslService, FirmwareVersion_500>(),

        };
};
