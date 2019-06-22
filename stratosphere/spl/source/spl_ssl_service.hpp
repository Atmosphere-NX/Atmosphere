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

    class SslService : public RsaService {
        public:
            SslService() : RsaService() { /* ... */ }
            virtual ~SslService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ImportSslKey(InPointer<u8> src, AccessKey access_key, KeySource key_source);
            virtual Result SslExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMetaEx<CommandId::GetConfig, &SslService::GetConfig, SslService>(),
                MakeServiceCommandMetaEx<CommandId::ExpMod, &SslService::ExpMod, SslService>(),
                MakeServiceCommandMetaEx<CommandId::SetConfig, &SslService::SetConfig, SslService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateRandomBytes, &SslService::GenerateRandomBytes, SslService>(),
                MakeServiceCommandMetaEx<CommandId::IsDevelopment, &SslService::IsDevelopment, SslService>(),
                MakeServiceCommandMetaEx<CommandId::SetBootReason, &SslService::SetBootReason, SslService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GetBootReason, &SslService::GetBootReason, SslService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKek, &SslService::GenerateAesKek, SslService>(),
                MakeServiceCommandMetaEx<CommandId::LoadAesKey, &SslService::LoadAesKey, SslService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKey, &SslService::GenerateAesKey, SslService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptAesKey, &SslService::DecryptAesKey, SslService>(),
                MakeServiceCommandMetaEx<CommandId::CryptAesCtr, &SslService::CryptAesCtr, SslService>(),
                MakeServiceCommandMetaEx<CommandId::ComputeCmac, &SslService::ComputeCmac, SslService>(),
                MakeServiceCommandMetaEx<CommandId::AllocateAesKeyslot, &SslService::AllocateAesKeyslot, SslService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::FreeAesKeyslot, &SslService::FreeAesKeyslot, SslService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::GetAesKeyslotAvailableEvent, &SslService::GetAesKeyslotAvailableEvent, SslService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &SslService::DecryptRsaPrivateKeyDeprecated, SslService, FirmwareVersion_400, FirmwareVersion_400>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &SslService::DecryptRsaPrivateKey, SslService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::ImportSslKey, &SslService::ImportSslKey, SslService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::SslExpMod, &SslService::SslExpMod, SslService, FirmwareVersion_500>(),

            };
    };

}
