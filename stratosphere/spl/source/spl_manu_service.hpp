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

class ManuService : public RsaService {
    public:
        ManuService(SecureMonitorWrapper *sw) : RsaService(sw) {
            /* ... */
        }

        virtual ~ManuService() {
            /* ... */
        }
    protected:
        /* Actual commands. */
        virtual Result ReEncryptRsaPrivateKey(OutPointerWithClientSize<u8> out, InPointer<u8> src, AccessKey access_key_dec, KeySource source_dec, AccessKey access_key_enc, KeySource source_enc, u32 option);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMetaEx<Spl_Cmd_GetConfig, &ManuService::GetConfig, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ExpMod, &ManuService::ExpMod, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetConfig, &ManuService::SetConfig, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateRandomBytes, &ManuService::GenerateRandomBytes, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_IsDevelopment, &ManuService::IsDevelopment, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetBootReason, &ManuService::SetBootReason, ManuService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetBootReason, &ManuService::GetBootReason, ManuService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKek, &ManuService::GenerateAesKek, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadAesKey, &ManuService::LoadAesKey, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKey, &ManuService::GenerateAesKey, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptAesKey, &ManuService::DecryptAesKey, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_CryptAesCtr, &ManuService::CryptAesCtr, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ComputeCmac, &ManuService::ComputeCmac, ManuService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_AllocateAesKeyslot, &ManuService::AllocateAesKeyslot, ManuService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_FreeAesKeyslot, &ManuService::FreeAesKeyslot, ManuService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetAesKeyslotAvailableEvent, &ManuService::GetAesKeyslotAvailableEvent, ManuService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &ManuService::DecryptRsaPrivateKeyDeprecated, ManuService, FirmwareVersion_400, FirmwareVersion_400>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &ManuService::DecryptRsaPrivateKey, ManuService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ReEncryptRsaPrivateKey, &ManuService::ReEncryptRsaPrivateKey, ManuService, FirmwareVersion_500>(),

        };
};
