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

class EsService : public RsaService {
    public:
        EsService(SecureMonitorWrapper *sw) : RsaService(sw) {
            /* ... */
        }

        virtual ~EsService() {
            /* ... */
        }
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
            MakeServiceCommandMetaEx<Spl_Cmd_GetConfig, &EsService::GetConfig, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ExpMod, &EsService::ExpMod, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetConfig, &EsService::SetConfig, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateRandomBytes, &EsService::GenerateRandomBytes, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_IsDevelopment, &EsService::IsDevelopment, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetBootReason, &EsService::SetBootReason, EsService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetBootReason, &EsService::GetBootReason, EsService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKek, &EsService::GenerateAesKek, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadAesKey, &EsService::LoadAesKey, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKey, &EsService::GenerateAesKey, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptAesKey, &EsService::DecryptAesKey, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_CryptAesCtr, &EsService::CryptAesCtr, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ComputeCmac, &EsService::ComputeCmac, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_AllocateAesKeyslot, &EsService::AllocateAesKeyslot, EsService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_FreeAesKeyslot, &EsService::FreeAesKeyslot, EsService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetAesKeyslotAvailableEvent, &EsService::GetAesKeyslotAvailableEvent, EsService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &EsService::DecryptRsaPrivateKeyDeprecated, EsService, FirmwareVersion_400, FirmwareVersion_400>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptRsaPrivateKey, &EsService::DecryptRsaPrivateKey, EsService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ImportEsKey, &EsService::ImportEsKey, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_UnwrapTitleKey, &EsService::UnwrapTitleKey, EsService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_UnwrapCommonTitleKey, &EsService::UnwrapCommonTitleKey, EsService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ImportDrmKey, &EsService::ImportDrmKey, EsService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DrmExpMod, &EsService::DrmExpMod, EsService, FirmwareVersion_500>(),
            MakeServiceCommandMetaEx<Spl_Cmd_UnwrapElicenseKey, &EsService::UnwrapElicenseKey, EsService, FirmwareVersion_600>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadElicenseKey, &EsService::LoadElicenseKey, EsService, FirmwareVersion_600>(),
        };
};
