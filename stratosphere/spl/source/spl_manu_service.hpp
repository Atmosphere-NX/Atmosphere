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
#include <stratosphere/spl/spl_types.hpp>

#include "spl_rsa_service.hpp"

namespace sts::spl {

    class ManuService : public RsaService {
        public:
            ManuService() : RsaService() { /* ... */ }

            virtual ~ManuService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ReEncryptRsaPrivateKey(OutPointerWithClientSize<u8> out, InPointer<u8> src, AccessKey access_key_dec, KeySource source_dec, AccessKey access_key_enc, KeySource source_enc, u32 option);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMetaEx<CommandId::GetConfig, &ManuService::GetConfig, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::ExpMod, &ManuService::ExpMod, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::SetConfig, &ManuService::SetConfig, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateRandomBytes, &ManuService::GenerateRandomBytes, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::IsDevelopment, &ManuService::IsDevelopment, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::SetBootReason, &ManuService::SetBootReason, ManuService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GetBootReason, &ManuService::GetBootReason, ManuService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKek, &ManuService::GenerateAesKek, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::LoadAesKey, &ManuService::LoadAesKey, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKey, &ManuService::GenerateAesKey, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptAesKey, &ManuService::DecryptAesKey, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::CryptAesCtr, &ManuService::CryptAesCtr, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::ComputeCmac, &ManuService::ComputeCmac, ManuService>(),
                MakeServiceCommandMetaEx<CommandId::AllocateAesKeyslot, &ManuService::AllocateAesKeyslot, ManuService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::FreeAesKeyslot, &ManuService::FreeAesKeyslot, ManuService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::GetAesKeyslotAvailableEvent, &ManuService::GetAesKeyslotAvailableEvent, ManuService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &ManuService::DecryptRsaPrivateKeyDeprecated, ManuService, FirmwareVersion_400, FirmwareVersion_400>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &ManuService::DecryptRsaPrivateKey, ManuService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::ReEncryptRsaPrivateKey, &ManuService::ReEncryptRsaPrivateKey, ManuService, FirmwareVersion_500>(),
            };
    };

}
