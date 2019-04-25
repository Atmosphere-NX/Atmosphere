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
        virtual Result DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &RsaService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &RsaService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &RsaService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &RsaService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &RsaService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &RsaService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &RsaService::GetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &RsaService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &RsaService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &RsaService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &RsaService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &RsaService::CryptAesCtr>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &RsaService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &RsaService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &RsaService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &RsaService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &RsaService::DecryptRsaPrivateKey>(),

        };
};
