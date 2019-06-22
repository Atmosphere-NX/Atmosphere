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

    class EsService : public RsaService {
        public:
            EsService() : RsaService() { /* ... */ }
            virtual ~EsService() { /* ... */}
        protected:
            /* Actual commands. */
            virtual Result ImportEsKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result UnwrapTitleKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation);
            virtual Result UnwrapCommonTitleKey(Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
            virtual Result ImportDrmKey(InPointer<u8> src, AccessKey access_key, KeySource key_source);
            virtual Result DrmExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod);
            virtual Result UnwrapElicenseKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation);
            virtual Result LoadElicenseKey(u32 keyslot, AccessKey access_key);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMetaEx<CommandId::GetConfig, &EsService::GetConfig, EsService>(),
                MakeServiceCommandMetaEx<CommandId::ExpMod, &EsService::ExpMod, EsService>(),
                MakeServiceCommandMetaEx<CommandId::SetConfig, &EsService::SetConfig, EsService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateRandomBytes, &EsService::GenerateRandomBytes, EsService>(),
                MakeServiceCommandMetaEx<CommandId::IsDevelopment, &EsService::IsDevelopment, EsService>(),
                MakeServiceCommandMetaEx<CommandId::SetBootReason, &EsService::SetBootReason, EsService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GetBootReason, &EsService::GetBootReason, EsService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKek, &EsService::GenerateAesKek, EsService>(),
                MakeServiceCommandMetaEx<CommandId::LoadAesKey, &EsService::LoadAesKey, EsService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKey, &EsService::GenerateAesKey, EsService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptAesKey, &EsService::DecryptAesKey, EsService>(),
                MakeServiceCommandMetaEx<CommandId::CryptAesCtr, &EsService::CryptAesCtr, EsService>(),
                MakeServiceCommandMetaEx<CommandId::ComputeCmac, &EsService::ComputeCmac, EsService>(),
                MakeServiceCommandMetaEx<CommandId::AllocateAesKeyslot, &EsService::AllocateAesKeyslot, EsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::FreeAesKeyslot, &EsService::FreeAesKeyslot, EsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::GetAesKeyslotAvailableEvent, &EsService::GetAesKeyslotAvailableEvent, EsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &EsService::DecryptRsaPrivateKeyDeprecated, EsService, FirmwareVersion_400, FirmwareVersion_400>(),
                MakeServiceCommandMetaEx<CommandId::DecryptRsaPrivateKey, &EsService::DecryptRsaPrivateKey, EsService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::ImportEsKey, &EsService::ImportEsKey, EsService>(),
                MakeServiceCommandMetaEx<CommandId::UnwrapTitleKey, &EsService::UnwrapTitleKey, EsService>(),
                MakeServiceCommandMetaEx<CommandId::UnwrapCommonTitleKey, &EsService::UnwrapCommonTitleKey, EsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::ImportDrmKey, &EsService::ImportDrmKey, EsService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::DrmExpMod, &EsService::DrmExpMod, EsService, FirmwareVersion_500>(),
                MakeServiceCommandMetaEx<CommandId::UnwrapElicenseKey, &EsService::UnwrapElicenseKey, EsService, FirmwareVersion_600>(),
                MakeServiceCommandMetaEx<CommandId::LoadElicenseKey, &EsService::LoadElicenseKey, EsService, FirmwareVersion_600>(),
            };
    };

}
