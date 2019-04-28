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
#include "spl_general_service.hpp"

class CryptoService : public GeneralService {
    public:
        CryptoService(SecureMonitorWrapper *sw) : GeneralService(sw) {
            /* ... */
        }

        virtual ~CryptoService() {
            this->GetSecureMonitorWrapper()->FreeAesKeyslots(this);
        }
    protected:
        /* Actual commands. */
        virtual Result GenerateAesKek(Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
        virtual Result LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source);
        virtual Result GenerateAesKey(Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
        virtual Result DecryptAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
        virtual Result CryptAesCtr(OutBuffer<u8, BufferType_Type1> out_buf, u32 keyslot, InBuffer<u8, BufferType_Type1> in_buf, IvCtr iv_ctr);
        virtual Result ComputeCmac(Out<Cmac> out_cmac, u32 keyslot, InPointer<u8> in_buf);
        virtual Result AllocateAesKeyslot(Out<u32> out_keyslot);
        virtual Result FreeAesKeyslot(u32 keyslot);
        virtual void GetAesKeyslotAvailableEvent(Out<CopiedHandle> out_hnd);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMetaEx<Spl_Cmd_GetConfig, &CryptoService::GetConfig, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ExpMod, &CryptoService::ExpMod, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetConfig, &CryptoService::SetConfig, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateRandomBytes, &CryptoService::GenerateRandomBytes, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_IsDevelopment, &CryptoService::IsDevelopment, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_SetBootReason, &CryptoService::SetBootReason, CryptoService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetBootReason, &CryptoService::GetBootReason, CryptoService, FirmwareVersion_300>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKek, &CryptoService::GenerateAesKek, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_LoadAesKey, &CryptoService::LoadAesKey, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GenerateAesKey, &CryptoService::GenerateAesKey, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_DecryptAesKey, &CryptoService::DecryptAesKey, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_CryptAesCtr, &CryptoService::CryptAesCtr, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_ComputeCmac, &CryptoService::ComputeCmac, CryptoService>(),
            MakeServiceCommandMetaEx<Spl_Cmd_AllocateAesKeyslot, &CryptoService::AllocateAesKeyslot, CryptoService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_FreeAesKeyslot, &CryptoService::FreeAesKeyslot, CryptoService, FirmwareVersion_200>(),
            MakeServiceCommandMetaEx<Spl_Cmd_GetAesKeyslotAvailableEvent, &CryptoService::GetAesKeyslotAvailableEvent, CryptoService, FirmwareVersion_200>(),

        };
};
