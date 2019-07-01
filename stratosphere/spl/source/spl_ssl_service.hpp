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

#include "spl_rsa_service.hpp"

namespace sts::spl {

    class SslService : public RsaService {
        public:
            SslService() : RsaService() { /* ... */ }
            virtual ~SslService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ImportSslKey(InPointer<u8> src, AccessKey access_key, KeySource key_source);
            virtual Result SslExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(SslService, GetConfig),
                MAKE_SERVICE_COMMAND_META(SslService, ExpMod),
                MAKE_SERVICE_COMMAND_META(SslService, SetConfig),
                MAKE_SERVICE_COMMAND_META(SslService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(SslService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(SslService, SetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(SslService, GetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(SslService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(SslService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(SslService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(SslService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(SslService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(SslService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(SslService, AllocateAesKeyslot,             FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(SslService, FreeAesKeyslot,                 FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(SslService, GetAesKeyslotAvailableEvent,    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(SslService, DecryptRsaPrivateKeyDeprecated, FirmwareVersion_400, FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(SslService, DecryptRsaPrivateKey,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(SslService, ImportSslKey,                   FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(SslService, SslExpMod,                      FirmwareVersion_500),

            };
    };

}
