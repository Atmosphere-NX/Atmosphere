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
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &SslService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &SslService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &SslService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &SslService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &SslService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &SslService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &SslService::GetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &SslService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &SslService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &SslService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &SslService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &SslService::CryptAesCtr>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &SslService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &SslService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &SslService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &SslService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &SslService::DecryptRsaPrivateKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &SslService::ImportSslKey, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &SslService::SslExpMod, FirmwareVersion_500>(),

        };
};
