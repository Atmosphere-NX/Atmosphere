/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::spl {

    class DeprecatedService : public sf::IServiceObject {
        protected:
            enum class CommandId {
                /* 1.0.0+ */
                GetConfig                      = 0,
                ExpMod                         = 1,
                GenerateAesKek                 = 2,
                LoadAesKey                     = 3,
                GenerateAesKey                 = 4,
                SetConfig                      = 5,
                GenerateRandomBytes            = 7,
                ImportLotusKey                 = 9,
                DecryptLotusMessage            = 10,
                IsDevelopment                  = 11,
                GenerateSpecificAesKey         = 12,
                DecryptRsaPrivateKeyDeprecated = 13,
                DecryptRsaPrivateKey           = 13,
                DecryptAesKey                  = 14,
                CryptAesCtrDeprecated          = 15,
                CryptAesCtr                    = 15,
                ComputeCmac                    = 16,
                ImportEsKey                    = 17,
                UnwrapTitleKeyDeprecated       = 18,
                UnwrapTitleKey                 = 18,
                LoadTitleKey                   = 19,

                /* 2.0.0+ */
                UnwrapCommonTitleKeyDeprecated = 20,
                UnwrapCommonTitleKey           = 20,
                AllocateAesKeyslot             = 21,
                FreeAesKeyslot                 = 22,
                GetAesKeyslotAvailableEvent    = 23,

                /* 3.0.0+ */
                SetBootReason                  = 24,
                GetBootReason                  = 25,
            };
        public:
            DeprecatedService() { /* ... */ }
            virtual ~DeprecatedService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result GetConfig(sf::Out<u64> out, u32 which);
            virtual Result ExpMod(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod);
            virtual Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
            virtual Result LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source);
            virtual Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
            virtual Result SetConfig(u32 which, u64 value);
            virtual Result GenerateRandomBytes(const sf::OutPointerBuffer &out);
            virtual Result ImportLotusKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptLotusMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            virtual Result IsDevelopment(sf::Out<bool> is_dev);
            virtual Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            virtual Result DecryptRsaPrivateKey(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
            virtual Result CryptAesCtrDeprecated(const sf::OutBuffer &out_buf, u32 keyslot, const sf::InBuffer &in_buf, IvCtr iv_ctr);
            virtual Result CryptAesCtr(const sf::OutNonSecureBuffer &out_buf, u32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr);
            virtual Result ComputeCmac(sf::Out<Cmac> out_cmac, u32 keyslot, const sf::InPointerBuffer &in_buf);
            virtual Result ImportEsKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result UnwrapTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            virtual Result UnwrapTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation);
            virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
            virtual Result UnwrapCommonTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, KeySource key_source);
            virtual Result UnwrapCommonTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
            virtual Result AllocateAesKeyslot(sf::Out<u32> out_keyslot);
            virtual Result FreeAesKeyslot(u32 keyslot);
            virtual void GetAesKeyslotAvailableEvent(sf::OutCopyHandle out_hnd);
            virtual Result SetBootReason(BootReasonValue boot_reason);
            virtual Result GetBootReason(sf::Out<BootReasonValue> out);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetConfig),
                MAKE_SERVICE_COMMAND_META(ExpMod),
                MAKE_SERVICE_COMMAND_META(GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(LoadAesKey),
                MAKE_SERVICE_COMMAND_META(GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(SetConfig),
                MAKE_SERVICE_COMMAND_META(GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(ImportLotusKey),
                MAKE_SERVICE_COMMAND_META(DecryptLotusMessage),
                MAKE_SERVICE_COMMAND_META(IsDevelopment),
                MAKE_SERVICE_COMMAND_META(GenerateSpecificAesKey),
                MAKE_SERVICE_COMMAND_META(DecryptRsaPrivateKey),
                MAKE_SERVICE_COMMAND_META(DecryptAesKey),

                MAKE_SERVICE_COMMAND_META(CryptAesCtrDeprecated,          hos::Version_1_0_0, hos::Version_1_0_0),
                MAKE_SERVICE_COMMAND_META(CryptAesCtr,                    hos::Version_2_0_0),

                MAKE_SERVICE_COMMAND_META(ComputeCmac),
                MAKE_SERVICE_COMMAND_META(ImportEsKey),

                MAKE_SERVICE_COMMAND_META(UnwrapTitleKeyDeprecated,       hos::Version_1_0_0, hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(UnwrapTitleKey,                 hos::Version_3_0_0),

                MAKE_SERVICE_COMMAND_META(LoadTitleKey),

                MAKE_SERVICE_COMMAND_META(UnwrapCommonTitleKeyDeprecated, hos::Version_2_0_0, hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(UnwrapCommonTitleKey,           hos::Version_3_0_0),

                MAKE_SERVICE_COMMAND_META(AllocateAesKeyslot,             hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(FreeAesKeyslot,                 hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetAesKeyslotAvailableEvent,    hos::Version_2_0_0),

                MAKE_SERVICE_COMMAND_META(SetBootReason,                  hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GetBootReason,                  hos::Version_3_0_0),
            };
    };

}
