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

namespace sts::spl {

    class GeneralService : public IServiceObject {
        protected:
            enum class CommandId {
                /* 1.0.0+ */
                GetConfig                      = 0,
                ExpMod                         = 1,
                GenerateAesKek                 = 2,
                LoadAesKey                     = 3,
                GenerateAesKey                 = 4,
                SetConfig                      = 5,
                GenerateRandomBytes            = 7,
                ImportLotusKey                 = 9,
                DecryptLotusMessage            = 10,
                IsDevelopment                  = 11,
                GenerateSpecificAesKey         = 12,
                DecryptRsaPrivateKeyDeprecated = 13,
                DecryptRsaPrivateKey           = 13,
                DecryptAesKey                  = 14,
                CryptAesCtr                    = 15,
                ComputeCmac                    = 16,
                ImportEsKey                    = 17,
                UnwrapTitleKey                 = 18,
                LoadTitleKey                   = 19,

                /* 2.0.0+ */
                UnwrapCommonTitleKey           = 20,
                AllocateAesKeyslot             = 21,
                FreeAesKeyslot                 = 22,
                GetAesKeyslotAvailableEvent    = 23,

                /* 3.0.0+ */
                SetBootReason                  = 24,
                GetBootReason                  = 25,

                /* 5.0.0+ */
                ImportSslKey                   = 26,
                SslExpMod                      = 27,
                ImportDrmKey                   = 28,
                DrmExpMod                      = 29,
                ReEncryptRsaPrivateKey         = 30,
                GetPackage2Hash                = 31,

                /* 6.0.0+ */
                UnwrapElicenseKey              = 31, /* re-used command id :( */
                LoadElicenseKey                = 32,
            };
        public:
            GeneralService() { /* ... */ }
            virtual ~GeneralService() { /* ... */ }
        protected:
            /* Actual commands. */
            virtual Result GetConfig(Out<u64> out, u32 which);
            virtual Result ExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> exp, InPointer<u8> mod);
            virtual Result SetConfig(u32 which, u64 value);
            virtual Result GenerateRandomBytes(OutPointerWithClientSize<u8> out);
            virtual Result IsDevelopment(Out<bool> is_dev);
            virtual Result SetBootReason(BootReasonValue boot_reason);
            virtual Result GetBootReason(Out<BootReasonValue> out);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GeneralService, GetConfig),
                MAKE_SERVICE_COMMAND_META(GeneralService, ExpMod),
                MAKE_SERVICE_COMMAND_META(GeneralService, SetConfig),
                MAKE_SERVICE_COMMAND_META(GeneralService, GenerateRandomBytes),
                MAKE_SERVICE_COMMAND_META(GeneralService, IsDevelopment),
                MAKE_SERVICE_COMMAND_META(GeneralService, SetBootReason,        FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(GeneralService, GetBootReason,        FirmwareVersion_300),
            };
    };

}
