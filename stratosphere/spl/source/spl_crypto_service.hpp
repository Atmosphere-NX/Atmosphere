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
#include "spl_general_service.hpp"

namespace ams::spl {

    class CryptoService : public GeneralService {
        public:
            CryptoService() : GeneralService() { /* ... */ }
            virtual ~CryptoService();
        protected:
            /* Actual commands. */
            virtual Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
            virtual Result LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source);
            virtual Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
            virtual Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
            virtual Result CryptAesCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr);
            virtual Result ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf);
            virtual Result AllocateAesKeyslot(sf::Out<s32> out_keyslot);
            virtual Result FreeAesKeyslot(s32 keyslot);
            virtual void GetAesKeyslotAvailableEvent(sf::OutCopyHandle out_hnd);
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
                MAKE_SERVICE_COMMAND_META(AllocateAesKeyslot           /* Atmosphere extension: This was added in hos::Version_2_0_0, but is allowed on older firmware by atmosphere. */),
                MAKE_SERVICE_COMMAND_META(FreeAesKeyslot               /* Atmosphere extension: This was added in hos::Version_2_0_0, but is allowed on older firmware by atmosphere. */),
                MAKE_SERVICE_COMMAND_META(GetAesKeyslotAvailableEvent  /* Atmosphere extension: This was added in hos::Version_2_0_0, but is allowed on older firmware by atmosphere. */),
            };
    };

}
