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
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &ManuService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &ManuService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &ManuService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &ManuService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &ManuService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &ManuService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &ManuService::GetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &ManuService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &ManuService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &ManuService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &ManuService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &ManuService::CryptAesCtr>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &ManuService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &ManuService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &ManuService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &ManuService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &ManuService::DecryptRsaPrivateKey>(),
            MakeServiceCommandMeta<Spl_Cmd_ReEncryptRsaPrivateKey, &ManuService::ReEncryptRsaPrivateKey, FirmwareVersion_500>(),

        };
};
