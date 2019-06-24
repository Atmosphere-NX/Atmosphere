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

namespace sts::spl {

    class DeprecatedService : public IServiceObject {
        protected:
            enum class CommandId {
                /* 1.0.0+ */
                GetConfig                   = 0,
                ExpMod                      = 1,
                GenerateAesKek              = 2,
                LoadAesKey                  = 3,
                GenerateAesKey              = 4,
                SetConfig                   = 5,
                GenerateRandomBytes         = 7,
                ImportLotusKey              = 9,
                DecryptLotusMessage         = 10,
                IsDevelopment               = 11,
                GenerateSpecificAesKey      = 12,
                DecryptRsaPrivateKey        = 13,
                DecryptAesKey               = 14,
                CryptAesCtr                 = 15,
                ComputeCmac                 = 16,
                ImportEsKey                 = 17,
                UnwrapTitleKey              = 18,
                LoadTitleKey                = 19,

                /* 2.0.0+ */
                UnwrapCommonTitleKey        = 20,
                AllocateAesKeyslot          = 21,
                FreeAesKeyslot              = 22,
                GetAesKeyslotAvailableEvent = 23,

                /* 3.0.0+ */
                SetBootReason               = 24,
                GetBootReason               = 25,
            };
        public:
            DeprecatedService() { /* ... */ }
            virtual ~DeprecatedService() { /* ... */ }
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
                MakeServiceCommandMeta<CommandId::GetConfig, &DeprecatedService::GetConfig>(),
                MakeServiceCommandMeta<CommandId::ExpMod, &DeprecatedService::ExpMod>(),
                MakeServiceCommandMeta<CommandId::GenerateAesKek, &DeprecatedService::GenerateAesKek>(),
                MakeServiceCommandMeta<CommandId::LoadAesKey, &DeprecatedService::LoadAesKey>(),
                MakeServiceCommandMeta<CommandId::GenerateAesKey, &DeprecatedService::GenerateAesKey>(),
                MakeServiceCommandMeta<CommandId::SetConfig, &DeprecatedService::SetConfig>(),
                MakeServiceCommandMeta<CommandId::GenerateRandomBytes, &DeprecatedService::GenerateRandomBytes>(),
                MakeServiceCommandMeta<CommandId::ImportLotusKey, &DeprecatedService::ImportLotusKey>(),
                MakeServiceCommandMeta<CommandId::DecryptLotusMessage, &DeprecatedService::DecryptLotusMessage>(),
                MakeServiceCommandMeta<CommandId::IsDevelopment, &DeprecatedService::IsDevelopment>(),
                MakeServiceCommandMeta<CommandId::GenerateSpecificAesKey, &DeprecatedService::GenerateSpecificAesKey>(),
                MakeServiceCommandMeta<CommandId::DecryptRsaPrivateKey, &DeprecatedService::DecryptRsaPrivateKey>(),
                MakeServiceCommandMeta<CommandId::DecryptAesKey, &DeprecatedService::DecryptAesKey>(),
                MakeServiceCommandMeta<CommandId::CryptAesCtr, &DeprecatedService::CryptAesCtrDeprecated, FirmwareVersion_100, FirmwareVersion_100>(),
                MakeServiceCommandMeta<CommandId::CryptAesCtr, &DeprecatedService::CryptAesCtr, FirmwareVersion_200>(),
                MakeServiceCommandMeta<CommandId::ComputeCmac, &DeprecatedService::ComputeCmac>(),
                MakeServiceCommandMeta<CommandId::ImportEsKey, &DeprecatedService::ImportEsKey>(),
                MakeServiceCommandMeta<CommandId::UnwrapTitleKey, &DeprecatedService::UnwrapTitleKey>(),
                MakeServiceCommandMeta<CommandId::LoadTitleKey, &DeprecatedService::LoadTitleKey>(),
                MakeServiceCommandMeta<CommandId::UnwrapCommonTitleKey, &DeprecatedService::UnwrapCommonTitleKey, FirmwareVersion_200>(),
                MakeServiceCommandMeta<CommandId::AllocateAesKeyslot, &DeprecatedService::AllocateAesKeyslot, FirmwareVersion_200>(),
                MakeServiceCommandMeta<CommandId::FreeAesKeyslot, &DeprecatedService::FreeAesKeyslot, FirmwareVersion_200>(),
                MakeServiceCommandMeta<CommandId::GetAesKeyslotAvailableEvent, &DeprecatedService::GetAesKeyslotAvailableEvent, FirmwareVersion_200>(),
                MakeServiceCommandMeta<CommandId::SetBootReason, &DeprecatedService::SetBootReason, FirmwareVersion_300>(),
                MakeServiceCommandMeta<CommandId::GetBootReason, &DeprecatedService::GetBootReason, FirmwareVersion_300>(),
            };
    };

}
