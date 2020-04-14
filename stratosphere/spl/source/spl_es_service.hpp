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
#include "spl_rsa_service.hpp"

namespace ams::spl {

    class EsService : public RsaService {
        public:
            EsService() : RsaService() { /* ... */ }
            virtual ~EsService() { /* ... */}
        protected:
            /* Actual commands. */
            virtual Result ImportEsKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result ImportEsKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source);
            virtual Result UnwrapTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation);
            virtual Result UnwrapCommonTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
            virtual Result ImportDrmKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source);
            virtual Result DrmExpMod(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod);
            virtual Result UnwrapElicenseKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation);
            virtual Result LoadElicenseKey(u32 keyslot, AccessKey access_key);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetConfig),
                MAKE_SERVICE_COMMAND_META(ExpMod),
                MAKE_SERVICE_COMMAND_META(SetConfig),
                MAKE_SERVICE_COMMAND_META(GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(IsDevelopment),
                MAKE_SERVICE_COMMAND_META(SetBootReason,                  hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GetBootReason,                  hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(LoadAesKey),
                MAKE_SERVICE_COMMAND_META(GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(ComputeCmac),
                MAKE_SERVICE_COMMAND_META(AllocateAesKeyslot,             hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(FreeAesKeyslot,                 hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetAesKeyslotAvailableEvent,    hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(DecryptRsaPrivateKeyDeprecated, hos::Version_4_0_0, hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(DecryptRsaPrivateKey,           hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(ImportEsKeyDeprecated,          hos::Version_4_0_0, hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(ImportEsKey,                    hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(UnwrapTitleKey),
                MAKE_SERVICE_COMMAND_META(UnwrapCommonTitleKey,           hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(ImportDrmKey,                   hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(DrmExpMod,                      hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(UnwrapElicenseKey,              hos::Version_6_0_0),
                MAKE_SERVICE_COMMAND_META(LoadElicenseKey,                hos::Version_6_0_0),
            };
    };

}
