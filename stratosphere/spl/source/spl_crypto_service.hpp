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

#include "spl_general_service.hpp"

namespace sts::spl {

    class CryptoService : public GeneralService {
        public:
            CryptoService() : GeneralService() { /* ... */ }
            virtual ~CryptoService();
        protected:
            /* Actual commands. */
            virtual Result GenerateAesKek(Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
            virtual Result LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source);
            virtual Result GenerateAesKey(Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
            virtual Result DecryptAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
            virtual Result CryptAesCtr(OutBuffer<u8, BufferType_Type1> out_buf, u32 keyslot, InBuffer<u8, BufferType_Type1> in_buf, IvCtr iv_ctr);
            virtual Result ComputeCmac(Out<Cmac> out_cmac, u32 keyslot, InPointer<u8> in_buf);
            virtual Result AllocateAesKeyslot(Out<u32> out_keyslot);
            virtual Result FreeAesKeyslot(u32 keyslot);
            virtual void GetAesKeyslotAvailableEvent(Out<CopiedHandle> out_hnd);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(CryptoService, GetConfig),
                MAKE_SERVICE_COMMAND_META(CryptoService, ExpMod),
                MAKE_SERVICE_COMMAND_META(CryptoService, SetConfig),
                MAKE_SERVICE_COMMAND_META(CryptoService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(CryptoService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(CryptoService, SetBootReason,               FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(CryptoService, GetBootReason,               FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(CryptoService, GenerateAesKek),
                MAKE_SERVICE_COMMAND_META(CryptoService, LoadAesKey),
                MAKE_SERVICE_COMMAND_META(CryptoService, GenerateAesKey),
                MAKE_SERVICE_COMMAND_META(CryptoService, DecryptAesKey),
                MAKE_SERVICE_COMMAND_META(CryptoService, CryptAesCtr),
                MAKE_SERVICE_COMMAND_META(CryptoService, ComputeCmac),
                MAKE_SERVICE_COMMAND_META(CryptoService, AllocateAesKeyslot,          FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(CryptoService, FreeAesKeyslot,              FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(CryptoService, GetAesKeyslotAvailableEvent, FirmwareVersion_200),
            };
    };

}
