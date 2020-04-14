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
#include "spl_crypto_service.hpp"

namespace ams::spl {

    class FsService : public CryptoService {
        public:
            FsService() : CryptoService() { /* ... */ }
            virtual ~FsService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ImportLotusKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result ImportLotusKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source);
            virtual Result DecryptLotusMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            virtual Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
            virtual Result GetPackage2Hash(const sf::OutPointerBuffer &dst);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetConfig),
                MAKE_SERVICE_COMMAND_META(ExpMod),
                MAKE_SERVICE_COMMAND_META(SetConfig),
                MAKE_SERVICE_COMMAND_META(GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(IsDevelopment),
                MAKE_SERVICE_COMMAND_META(SetBootReason,               hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GetBootReason,               hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(LoadAesKey),
                MAKE_SERVICE_COMMAND_META(GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(ComputeCmac),
                MAKE_SERVICE_COMMAND_META(AllocateAesKeyslot,          hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(FreeAesKeyslot,              hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetAesKeyslotAvailableEvent, hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(ImportLotusKeyDeprecated,    hos::Version_4_0_0, hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(ImportLotusKey,              hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(DecryptLotusMessage),
                MAKE_SERVICE_COMMAND_META(GenerateSpecificAesKey),
                MAKE_SERVICE_COMMAND_META(LoadTitleKey),
                MAKE_SERVICE_COMMAND_META(GetPackage2Hash,             hos::Version_5_0_0),
            };
    };

}
