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
#include "spl_secmon_wrapper.hpp"

class DeprecatedService : public IServiceObject {
    private:
        SecureMonitorWrapper *secmon_wrapper;
    public:
        DeprecatedService(SecureMonitorWrapper *sw) : secmon_wrapper(sw) {
            /* ... */
        }

        virtual ~DeprecatedService() { /* ... */ }
    protected:
        SecureMonitorWrapper *GetSecureMonitorWrapper() const {
            return this->secmon_wrapper;
        }
    protected:
        /* Actual commands. */
        virtual Result GetConfig(Out<u64> out, u32 which);
        virtual Result ExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> exp, InPointer<u8> mod);
        virtual Result GenerateAesKek(Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
        virtual Result LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source);
        virtual Result GenerateAesKey(Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
        virtual Result SetConfig(u32 which, u64 value);
        virtual Result GenerateRandomBytes(OutPointerWithClientSize<u8> out);
        virtual Result ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
        virtual Result DecryptLotusMessage(Out<u32> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest);
        virtual Result IsDevelopment(Out<bool> is_dev);
        virtual Result GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
        virtual Result DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
        virtual Result DecryptAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
        virtual Result CryptAesCtrDeprecated(OutBuffer<u8> out_buf, u32 keyslot, InBuffer<u8> in_buf, IvCtr iv_ctr);
        virtual Result CryptAesCtr(OutBuffer<u8, BufferType_Type1> out_buf, u32 keyslot, InBuffer<u8, BufferType_Type1> in_buf, IvCtr iv_ctr);
        virtual Result ComputeCmac(Out<Cmac> out_cmac, u32 keyslot, InPointer<u8> in_buf);
        virtual Result ImportEsKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
        virtual Result UnwrapTitleKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation);
        virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
        virtual Result UnwrapCommonTitleKey(Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
        virtual Result AllocateAesKeyslot(Out<u32> out_keyslot);
        virtual Result FreeAesKeyslot(u32 keyslot);
        virtual void GetAesKeyslotAvailableEvent(Out<CopiedHandle> out_hnd);
        virtual Result SetBootReason(BootReasonValue boot_reason);
        virtual Result GetBootReason(Out<BootReasonValue> out);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &DeprecatedService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &DeprecatedService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKek, &DeprecatedService::GenerateAesKek>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadAesKey, &DeprecatedService::LoadAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateAesKey, &DeprecatedService::GenerateAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &DeprecatedService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &DeprecatedService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_ImportLotusKey, &DeprecatedService::ImportLotusKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptLotusMessage, &DeprecatedService::DecryptLotusMessage>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &DeprecatedService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateSpecificAesKey, &DeprecatedService::GenerateSpecificAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptRsaPrivateKey, &DeprecatedService::DecryptRsaPrivateKey>(),
            MakeServiceCommandMeta<Spl_Cmd_DecryptAesKey, &DeprecatedService::DecryptAesKey>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &DeprecatedService::CryptAesCtrDeprecated, FirmwareVersion_100, FirmwareVersion_100>(),
            MakeServiceCommandMeta<Spl_Cmd_CryptAesCtr, &DeprecatedService::CryptAesCtr, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_ComputeCmac, &DeprecatedService::ComputeCmac>(),
            MakeServiceCommandMeta<Spl_Cmd_ImportEsKey, &DeprecatedService::ImportEsKey>(),
            MakeServiceCommandMeta<Spl_Cmd_UnwrapTitleKey, &DeprecatedService::UnwrapTitleKey>(),
            MakeServiceCommandMeta<Spl_Cmd_LoadTitleKey, &DeprecatedService::LoadTitleKey>(),
            MakeServiceCommandMeta<Spl_Cmd_UnwrapCommonTitleKey, &DeprecatedService::UnwrapCommonTitleKey, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_AllocateAesKeyslot, &DeprecatedService::AllocateAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_FreeAesKeyslot, &DeprecatedService::FreeAesKeyslot, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_GetAesKeyslotAvailableEvent, &DeprecatedService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &DeprecatedService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &DeprecatedService::GetBootReason, FirmwareVersion_300>(),
        };
};
