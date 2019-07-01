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

    class ManuService : public RsaService {
        public:
            ManuService() : RsaService() { /* ... */ }

            virtual ~ManuService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ReEncryptRsaPrivateKey(OutPointerWithClientSize<u8> out, InPointer<u8> src, AccessKey access_key_dec, KeySource source_dec, AccessKey access_key_enc, KeySource source_enc, u32 option);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ManuService, GetConfig),
                MAKE_SERVICE_COMMAND_META(ManuService, ExpMod),
                MAKE_SERVICE_COMMAND_META(ManuService, SetConfig),
                MAKE_SERVICE_COMMAND_META(ManuService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(ManuService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(ManuService, SetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(ManuService, GetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(ManuService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(ManuService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(ManuService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(ManuService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(ManuService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(ManuService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(ManuService, AllocateAesKeyslot,             FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ManuService, FreeAesKeyslot,                 FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ManuService, GetAesKeyslotAvailableEvent,    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ManuService, DecryptRsaPrivateKeyDeprecated, FirmwareVersion_400, FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(ManuService, DecryptRsaPrivateKey,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ManuService, ReEncryptRsaPrivateKey,         FirmwareVersion_500),
            };
    };

}
