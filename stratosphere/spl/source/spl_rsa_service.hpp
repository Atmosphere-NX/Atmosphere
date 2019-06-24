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

#include "spl_crypto_service.hpp"

namespace sts::spl {

    class RsaService : public CryptoService {
        public:
            RsaService() : CryptoService() { /* ... */ }
            virtual ~RsaService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result DecryptRsaPrivateKeyDeprecated(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMetaEx<CommandId::GetConfig, &RsaService::GetConfig, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::ExpMod, &RsaService::ExpMod, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::SetConfig, &RsaService::SetConfig, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateRandomBytes, &RsaService::GenerateRandomBytes, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::IsDevelopment, &RsaService::IsDevelopment, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::SetBootReason, &RsaService::SetBootReason, RsaService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GetBootReason, &RsaService::GetBootReason, RsaService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKek, &RsaService::GenerateAesKek, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::LoadAesKey, &RsaService::LoadAesKey, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKey, &RsaService::GenerateAesKey, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptAesKey, &RsaService::DecryptAesKey, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::CryptAesCtr, &RsaService::CryptAesCtr, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::ComputeCmac, &RsaService::ComputeCmac, RsaService>(),
                MakeServiceCommandMetaEx<CommandId::AllocateAesKeyslot, &RsaService::AllocateAesKeyslot, RsaService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::FreeAesKeyslot, &RsaService::FreeAesKeyslot, RsaService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::GetAesKeyslotAvailableEvent, &RsaService::GetAesKeyslotAvailableEvent, RsaService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &RsaService::DecryptRsaPrivateKeyDeprecated, RsaService, FirmwareVersion_400, FirmwareVersion_400>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &RsaService::DecryptRsaPrivateKey, RsaService, FirmwareVersion_500>(),
            };
    };

}
