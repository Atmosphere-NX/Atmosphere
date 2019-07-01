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

    class EsService : public RsaService {
        public:
            EsService() : RsaService() { /* ... */ }
            virtual ~EsService() { /* ... */}
        protected:
            /* Actual commands. */
            virtual Result ImportEsKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result UnwrapTitleKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation);
            virtual Result UnwrapCommonTitleKey(Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
            virtual Result ImportDrmKey(InPointer<u8> src, AccessKey access_key, KeySource key_source);
            virtual Result DrmExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod);
            virtual Result UnwrapElicenseKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation);
            virtual Result LoadElicenseKey(u32 keyslot, AccessKey access_key);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(EsService, GetConfig),
                MAKE_SERVICE_COMMAND_META(EsService, ExpMod),
                MAKE_SERVICE_COMMAND_META(EsService, SetConfig),
                MAKE_SERVICE_COMMAND_META(EsService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(EsService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(EsService, SetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(EsService, GetBootReason,                  FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(EsService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(EsService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(EsService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(EsService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(EsService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(EsService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(EsService, AllocateAesKeyslot,             FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(EsService, FreeAesKeyslot,                 FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(EsService, GetAesKeyslotAvailableEvent,    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(EsService, DecryptRsaPrivateKeyDeprecated, FirmwareVersion_400, FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(EsService, DecryptRsaPrivateKey,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(EsService, ImportEsKey),
                MAKE_SERVICE_COMMAND_META(EsService, UnwrapTitleKey),
                MAKE_SERVICE_COMMAND_META(EsService, UnwrapCommonTitleKey,           FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(EsService, ImportDrmKey,                   FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(EsService, DrmExpMod,                      FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(EsService, UnwrapElicenseKey,              FirmwareVersion_600),
                MAKE_SERVICE_COMMAND_META(EsService, LoadElicenseKey,                FirmwareVersion_600),
            };
    };

}
