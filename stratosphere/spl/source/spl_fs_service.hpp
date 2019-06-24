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

    class FsService : public CryptoService {
        public:
            FsService() : CryptoService() { /* ... */ }
            virtual ~FsService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptLotusMessage(Out<u32> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest);
            virtual Result GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
            virtual Result GetPackage2Hash(OutPointerWithClientSize<u8> dst);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMetaEx<CommandId::GetConfig, &FsService::GetConfig, FsService>(),
                MakeServiceCommandMetaEx<CommandId::ExpMod, &FsService::ExpMod, FsService>(),
                MakeServiceCommandMetaEx<CommandId::SetConfig, &FsService::SetConfig, FsService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateRandomBytes, &FsService::GenerateRandomBytes, FsService>(),
                MakeServiceCommandMetaEx<CommandId::IsDevelopment, &FsService::IsDevelopment, FsService>(),
                MakeServiceCommandMetaEx<CommandId::SetBootReason, &FsService::SetBootReason, FsService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GetBootReason, &FsService::GetBootReason, FsService, FirmwareVersion_300>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKek, &FsService::GenerateAesKek, FsService>(),
                MakeServiceCommandMetaEx<CommandId::LoadAesKey, &FsService::LoadAesKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateAesKey, &FsService::GenerateAesKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptAesKey, &FsService::DecryptAesKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::CryptAesCtr, &FsService::CryptAesCtr, FsService>(),
                MakeServiceCommandMetaEx<CommandId::ComputeCmac, &FsService::ComputeCmac, FsService>(),
                MakeServiceCommandMetaEx<CommandId::AllocateAesKeyslot, &FsService::AllocateAesKeyslot, FsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::FreeAesKeyslot, &FsService::FreeAesKeyslot, FsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::GetAesKeyslotAvailableEvent, &FsService::GetAesKeyslotAvailableEvent, FsService, FirmwareVersion_200>(),
                MakeServiceCommandMetaEx<CommandId::ImportLotusKey, &FsService::ImportLotusKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::DecryptLotusMessage, &FsService::DecryptLotusMessage, FsService>(),
                MakeServiceCommandMetaEx<CommandId::GenerateSpecificAesKey, &FsService::GenerateSpecificAesKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::LoadTitleKey, &FsService::LoadTitleKey, FsService>(),
                MakeServiceCommandMetaEx<CommandId::GetPackage2Hash, &FsService::GetPackage2Hash, FsService, FirmwareVersion_500>(),
            };
    };

}
