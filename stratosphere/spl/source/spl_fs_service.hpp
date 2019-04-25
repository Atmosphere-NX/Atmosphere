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

class FsService : public CryptoService {
    public:
        FsService(SecureMonitorWrapper *sw) : CryptoService(sw) {
            /* ... */
        }

        virtual ~FsService() {
            /* ... */
        }
    protected:
        /* Actual commands. */
        virtual Result ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
        virtual Result DecryptLotusMessage(Out<size_t> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest);
        virtual Result GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
        virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
        virtual Result GetPackage2Hash(OutPointerWithClientSize<u8> dst);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &FsService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &FsService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &FsService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &FsService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &FsService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &FsService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &FsService::GetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &FsService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &FsService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &FsService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &FsService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &FsService::CryptAesCtr>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &FsService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &FsService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &FsService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &FsService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_ImportLotusKey, &FsService::ImportLotusKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptLotusMessage, &FsService::DecryptLotusMessage>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateSpecificAesKey, &FsService::GenerateSpecificAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadTitleKey, &FsService::LoadTitleKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GetPackage2Hash, &FsService::GetPackage2Hash, FirmwareVersion_500>(),

        };
};
