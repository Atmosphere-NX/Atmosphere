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
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <switch.h>
#include <stratosphere.hpp>

enum SmcResult : u32 {
    SmcResult_Success = 0,
    SmcResult_NotImplemented = 1,
    SmcResult_InvalidArgument = 2,
    SmcResult_InProgress = 3,
    SmcResult_NoAsyncOperation = 4,
    SmcResult_InvalidAsyncOperation = 5,
    SmcResult_Blacklisted = 6,

    SmcResult_Max = 99,
};

enum SmcCipherMode : u32 {
    SmcCipherMode_CbcEncrypt = 0,
    SmcCipherMode_CbcDecrypt = 1,
    SmcCipherMode_Ctr = 2,
};

enum SmcDecryptOrImportMode : u32 {
    SmcDecryptOrImportMode_DecryptRsaPrivateKey = 0,
    SmcDecryptOrImportMode_ImportLotusKey = 1,
    SmcDecryptOrImportMode_ImportEsKey = 2,
    SmcDecryptOrImportMode_ImportSslKey = 3,
    SmcDecryptOrImportMode_ImportDrmKey = 4,
};

enum SmcSecureExpModMode : u32 {
    SmcSecureExpModMode_Lotus = 0,
    SmcSecureExpModMode_Ssl = 1,
    SmcSecureExpModMode_Drm = 2,
};

enum EsKeyType : u32 {
    EsKeyType_TitleKey = 0,
    EsKeyType_ElicenseKey = 1,
};

struct AsyncOperationKey {
    u64 value;
};

struct BootReasonValue {
    u8 power_intr;
    u8 rtc_intr;
    u8 _0x3;
    u8 boot_reason;
};
static_assert(sizeof(BootReasonValue) == sizeof(u32), "BootReasonValue definition!");

#pragma pack(push, 1)
struct AesKey {
    union {
        u8 data[AES_128_KEY_SIZE];
        u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
    };
};
static_assert(alignof(AesKey) == alignof(u8), "AesKey definition!");

struct IvCtr {
    union {
        u8 data[AES_128_KEY_SIZE];
        u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
    };
};
static_assert(alignof(IvCtr) == alignof(u8), "IvCtr definition!");

struct Cmac {
    union {
        u8 data[AES_128_KEY_SIZE];
        u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
    };
};
static_assert(alignof(Cmac) == alignof(u8), "Cmac definition!");

struct AccessKey {
    union {
        u8 data[AES_128_KEY_SIZE];
        u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
    };
};
static_assert(alignof(AccessKey) == alignof(u8), "AccessKey definition!");

struct KeySource {
    union {
        u8 data[AES_128_KEY_SIZE];
        u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
    };
};
static_assert(alignof(AccessKey) == alignof(u8), "KeySource definition!");
#pragma pack(pop)

enum CsrngCmd {
    Csrng_Cmd_GenerateRandomBytes = 0,
};

enum SplServiceCmd {
    /* 1.0.0+ */
    Spl_Cmd_GetConfig = 0,
    Spl_Cmd_ExpMod = 1,
    Spl_Cmd_GenerateAesKek = 2,
    Spl_Cmd_LoadAesKey = 3,
    Spl_Cmd_GenerateAesKey = 4,
    Spl_Cmd_SetConfig = 5,
    Spl_Cmd_GenerateRandomBytes = 7,
    Spl_Cmd_ImportLotusKey = 9,
    Spl_Cmd_DecryptLotusMessage = 10,
    Spl_Cmd_IsDevelopment = 11,
    Spl_Cmd_GenerateSpecificAesKey = 12,
    Spl_Cmd_DecryptRsaPrivateKey = 13,
    Spl_Cmd_DecryptAesKey = 14,
    Spl_Cmd_CryptAesCtr = 15,
    Spl_Cmd_ComputeCmac = 16,
    Spl_Cmd_ImportEsKey = 17,
    Spl_Cmd_UnwrapTitleKey = 18,
    Spl_Cmd_LoadTitleKey = 19,

    /* 2.0.0+ */
    Spl_Cmd_UnwrapCommonTitleKey = 20,
    Spl_Cmd_AllocateAesKeyslot = 21,
    Spl_Cmd_FreeAesKeyslot = 22,
    Spl_Cmd_GetAesKeyslotAvailableEvent = 23,

    /* 3.0.0+ */
    Spl_Cmd_SetBootReason = 24,
    Spl_Cmd_GetBootReason = 25,

    /* 5.0.0+ */
    Spl_Cmd_ImportSslKey = 26,
    Spl_Cmd_SslExpMod = 27,
    Spl_Cmd_ImportDrmKey = 28,
    Spl_Cmd_DrmExpMod = 29,
    Spl_Cmd_ReEncryptRsaPrivateKey = 30,
    Spl_Cmd_GetPackage2Hash = 31,

    /* 6.0.0+ */
    Spl_Cmd_UnwrapElicenseKey = 31, /* re-used command id :( */
    Spl_Cmd_LoadElicenseKey = 32,
};
