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

    class RsaService : public CryptoService {
        public:
            RsaService() : CryptoService() { /* ... */ }
            virtual ~RsaService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result DecryptRsaPrivateKeyDeprecated(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(RsaService, GetConfig),
                MAKE_SERVICE_COMMAND_META(RsaService, ExpMod),
                MAKE_SERVICE_COMMAND_META(RsaService, SetConfig),
                MAKE_SERVICE_COMMAND_META(RsaService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(RsaService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(RsaService, SetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(RsaService, GetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(RsaService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(RsaService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(RsaService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(RsaService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(RsaService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(RsaService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(RsaService, AllocateAesKeyslot,             FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RsaService, FreeAesKeyslot,                 FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RsaService, GetAesKeyslotAvailableEvent,    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RsaService, DecryptRsaPrivateKeyDeprecated, FirmwareVersion_400, FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(RsaService, DecryptRsaPrivateKey,           FirmwareVersion_500),
            };
    };

}
