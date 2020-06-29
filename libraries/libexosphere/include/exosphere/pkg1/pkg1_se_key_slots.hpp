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
#include <vapours.hpp>

namespace ams::pkg1 {

    enum AesKeySlot {
        AesKeySlot_UserStart                      = 0,

        AesKeySlot_TzramSaveKek                   = 2,
        AesKeySlot_TzramSaveKey                   = 3,

        AesKeySlot_UserLast                       = 5,
        AesKeySlot_UserEnd                        = AesKeySlot_UserLast + 1,

        AesKeySlot_SecmonStart                    = 8,

        AesKeySlot_Temporary                      =  8,
        AesKeySlot_Smc                            =  9,
        AesKeySlot_RandomForUserWrap              = 10,
        AesKeySlot_RandomForKeyStorageWrap        = 11,
        AesKeySlot_DeviceMaster                   = 12,
        AesKeySlot_Master                         = 13,
        AesKeySlot_Device                         = 15,

        AesKeySlot_Count                          = 16,
        AesKeySlot_SecmonEnd                      = AesKeySlot_Count,

        /* Used only during boot. */
        AesKeySlot_Tsec                           = 12,
        AesKeySlot_TsecRoot                       = 13,
        AesKeySlot_SecureBoot                     = 14,
        AesKeySlot_SecureStorage                  = 15,

        AesKeySlot_DeviceMasterKeySourceKekErista = 10,
        AesKeySlot_MasterKek                      = 13,
        AesKeySlot_DeviceMasterKeySourceKekMariko = 14,

        /* Mariko only keyslots, used during boot. */
        AesKeySlot_MarikoKek                      = 12,
        AesKeySlot_MarikoBek                      = 13,
    };

    enum RsaKeySlot {
        RsaKeySlot_Temporary  = 0,
        RsaKeySlot_PrivateKey = 1,
    };

    constexpr bool IsUserAesKeySlot(int slot) {
        return AesKeySlot_UserStart <= slot && slot < AesKeySlot_UserEnd;
    }

}
