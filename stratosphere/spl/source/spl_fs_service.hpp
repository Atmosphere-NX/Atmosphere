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

    class FsService : public CryptoService {
        public:
            FsService() : CryptoService() { /* ... */ }
            virtual ~FsService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option);
            virtual Result DecryptLotusMessage(Out<u32> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest);
            virtual Result GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            virtual Result LoadTitleKey(u32 keyslot, AccessKey access_key);
            virtual Result GetPackage2Hash(OutPointerWithClientSize<u8> dst);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(FsService, GetConfig),
                MAKE_SERVICE_COMMAND_META(FsService, ExpMod),
                MAKE_SERVICE_COMMAND_META(FsService, SetConfig),
                MAKE_SERVICE_COMMAND_META(FsService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(FsService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(FsService, SetBootReason,               FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(FsService, GetBootReason,               FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(FsService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(FsService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(FsService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(FsService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(FsService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(FsService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(FsService, AllocateAesKeyslot,          FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(FsService, FreeAesKeyslot,              FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(FsService, GetAesKeyslotAvailableEvent, FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(FsService, ImportLotusKey),
                MAKE_SERVICE_COMMAND_META(FsService, DecryptLotusMessage),
                MAKE_SERVICE_COMMAND_META(FsService, GenerateSpecificAesKey),
                MAKE_SERVICE_COMMAND_META(FsService, LoadTitleKey),
                MAKE_SERVICE_COMMAND_META(FsService, GetPackage2Hash,             FirmwareVersion_500),
            };
    };

}
