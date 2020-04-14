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

    class ManuService : public RsaService {
        public:
            ManuService() : RsaService() { /* ... */ }

            virtual ~ManuService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ReEncryptRsaPrivateKey(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &src, AccessKey access_key_dec, KeySource source_dec, AccessKey access_key_enc, KeySource source_enc, u32 option);
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
                MAKE_SERVICE_COMMAND_META(ReEncryptRsaPrivateKey,         hos::Version_5_0_0),
            };
    };

}
