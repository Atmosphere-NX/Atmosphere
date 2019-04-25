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
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &EsService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &EsService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &EsService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &EsService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &EsService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &EsService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &EsService::GetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &EsService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &EsService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &EsService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &EsService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &EsService::CryptAesCtr>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &EsService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &EsService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &EsService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &EsService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &EsService::DecryptRsaPrivateKey>(),
            MakeServiceCommandMeta<Spl_Cmd_ImportEsKey, &EsService::ImportEsKey>(),
            MakeServiceCommandMeta<Spl_Cmd_UnwrapTitleKey, &EsService::UnwrapTitleKey>(),
            MakeServiceCommandMeta<Spl_Cmd_UnwrapCommonTitleKey, &EsService::UnwrapCommonTitleKey, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_ImportDrmKey, &EsService::ImportDrmKey, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Spl_Cmd_DrmExpMod, &EsService::DrmExpMod, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Spl_Cmd_UnwrapElicenseKey, &EsService::UnwrapElicenseKey, FirmwareVersion_600>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadElicenseKey, &EsService::LoadElicenseKey, FirmwareVersion_600>(),
        };
};
